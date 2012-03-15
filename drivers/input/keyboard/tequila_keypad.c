/*
 *  GPIO driven tequila keyboard driver
 *
 *  Copyright (C) 2010-2011 Amazon Technologies, Inc. All Rights Reserved.
 *  Manish Lachwani (lachwani@lab126.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/input/tequila_keypad.h>
#include <linux/slab.h>

#include <linux/proc_fs.h>
#include <asm/uaccess.h>

struct tequila_keypad {
	const struct tequila_keypad_platform_data *pdata;
	struct input_dev *input_dev;
	unsigned short *keycodes;
	unsigned int row_shift;

	DECLARE_BITMAP(disabled_gpios, TEQUILA_MAX_ROWS);

	uint32_t last_key_state[TEQUILA_MAX_COLS];
	struct delayed_work work;
	spinlock_t lock;
	bool scan_pending;
	bool stopped;
	bool gpio_all_disabled;
};

static struct proc_dir_entry *proc_entry;
static int keypad_lock = 0;
static struct input_dev* teqkb_dev;
static int more_log = 0;

#include <mach/boardid.h>

#define PROC_CMD_LEN 50

/*!
 * This function returns the current state of the keyboard
 */
int keypad_proc_read( char *page, char **start, off_t off,
                      int count, int *eof, void *data )
{
   int len;

   if (off > 0) {
     *eof = 1;
     return 0;
   }
   
   len = sprintf(page, keypad_lock ? "keypad is locked\n":"keypad is unlocked\n");

   return len;
}

/**
* This is a wrapper for input_event.  In addition to calling
* input_event, this function also sends a uevent for any
* processes listening in the userspace
*/
void report_event(struct input_dev *dev, unsigned int type, unsigned int code, int value )
{
        // Send key input event
	input_event( dev, type, code, value );

	// Send uevent so userspace power daemon can monitor
	// keyboard activity.  NOTE - the keycode is not sent through
	// this medium; only the fact that a key event has happened.
	// NOTE: only sending uevent on key down.
	if (value == 1) {
	//	printk(KERN_INFO "report code %d\n", code);
     	   if( kobject_uevent_atomic( &dev->dev.kobj, KOBJ_CHANGE ) )
	   {
		printk(KERN_ERR "tequila_keypad: E uevent:teqkb::Keyboard failed to send uevent\n" );
	   }
	}
}

static int tequila_keypad_start(struct input_dev *dev);
static void tequila_keypad_stop(struct input_dev *dev);

/*!
 * This function executes keypad control commands based on the input string.
 *    unlock = unlock the keypad
 *    lock   = lock the keypad
 */
ssize_t keypad_proc_write( struct file *filp, const char __user *buff,
                           unsigned long len, void *data )

{
   char command[PROC_CMD_LEN];
   int  keycode = 0;

   if (len > PROC_CMD_LEN) {
        dev_err(&teqkb_dev->dev, "keypad_proc_write:proc_len::keypad command is too long!\n");
        return -ENOSPC;
   }

   if (copy_from_user(command, buff, len)) {
        dev_err(&teqkb_dev->dev,"keypad_proc_write:copy2user::cannot copy from user!\n");
        return -EFAULT;
   }
   if ( !strncmp(command, "unlock", 6) ) {
      // Requested to unlock keypad
      if (!keypad_lock) {
         // Keypad was already unlocked; do nothing
         if(more_log)
            dev_info(&teqkb_dev->dev, "keypad_proc_write:unlocked1:status=unlocked:\n");
      }
      else {

         // Unlock keypad
            tequila_keypad_start(teqkb_dev);
            keypad_lock = 0;
            if(more_log)
                dev_info(&teqkb_dev->dev,"keypad_proc_write:unlocked2:status=unlocked:\n");

      }
   }else if( !strncmp(command, "lock", 4) ) {
      // Requested to lock keypad
      if (keypad_lock) {
         // Keypad was already locked; do nothing
         if(more_log)
            dev_err(&teqkb_dev->dev,"keypad_proc_write:locked1:status=locked:\n");
      }
      else {

            // Lock keypad
            tequila_keypad_stop(teqkb_dev);
            keypad_lock = 1;
            if(more_log)
                dev_info(&teqkb_dev->dev,"keypad_proc_write:locked2:status=locked:\n");

      }
   }else  if( !strncmp(command, "send", 4) ) {
      // Requested to send a keycode
      if (keypad_lock) {
         // Keypad is locked; do nothing
         if(more_log)
            dev_warn(&teqkb_dev->dev,"keypad_proc_write:locked3::The keypad is locked!  Unlock before sending keycodes.\n");
      } else  {
         // Read keycode and send
        sscanf(command, "send %d", &keycode);
        if(more_log)
            dev_info(&teqkb_dev->dev,"keypad_proc_write:send_key::Sending keycode %d\n", keycode);
        report_event(teqkb_dev, EV_KEY, keycode, 1);
        report_event(teqkb_dev, EV_KEY, keycode, 0);
      }
   }else if(!strncmp(command, "more", 4) )
   {
        more_log = 1;
   }else if(!(strncmp(command, "less", 4)))
   {
        more_log = 0;
   }
   else{
       dev_err(&teqkb_dev->dev,"keypad_proc_write:proc_cmd:command=%s:Unrecognized keypad command\n", command);
   }
   return len;
}

/*
 * NOTE: normally the GPIO has to be put into HiZ when de-activated to cause
 * minmal side effect when scanning other columns, here it is configured to
 * be input, and it should work on most platforms.
 */
static void __activate_col(const struct tequila_keypad_platform_data *pdata,
			   int col, bool on)
{
	bool level_on = !pdata->active_low;

	if (on) {
		gpio_direction_output(pdata->col_gpios[col], level_on);
	} else {
		gpio_set_value_cansleep(pdata->col_gpios[col], !level_on);
		gpio_direction_input(pdata->col_gpios[col]);
	}
}

static void activate_col(const struct tequila_keypad_platform_data *pdata,
			 int col, bool on)
{
	__activate_col(pdata, col, on);

	if (on && pdata->col_scan_delay_us)
		udelay(pdata->col_scan_delay_us);
}

static void activate_all_cols(const struct tequila_keypad_platform_data *pdata,
			      bool on)
{
	int col;

	for (col = 0; col < pdata->num_col_gpios; col++)
		__activate_col(pdata, col, on);
}

static bool row_asserted(const struct tequila_keypad_platform_data *pdata,
			 int row)
{
	return gpio_get_value_cansleep(pdata->row_gpios[row]) ?
			!pdata->active_low : pdata->active_low;
}

static void enable_row_irqs(struct tequila_keypad *keypad)
{
	const struct tequila_keypad_platform_data *pdata = keypad->pdata;
	int i;

	if (pdata->clustered_irq > 0)
		enable_irq(pdata->clustered_irq);
	else {
		for (i = 0; i < pdata->num_row_gpios; i++)
			enable_irq(gpio_to_irq(pdata->row_gpios[i]));
	}
}

static void disable_row_irqs(struct tequila_keypad *keypad)
{
	const struct tequila_keypad_platform_data *pdata = keypad->pdata;
	int i;

	if (pdata->clustered_irq > 0)
		disable_irq_nosync(pdata->clustered_irq);
	else {
		for (i = 0; i < pdata->num_row_gpios; i++)
			disable_irq_nosync(gpio_to_irq(pdata->row_gpios[i]));
	}
}

/*
 * This gets the keys from keyboard and reports it to input subsystem
 */
static void tequila_keypad_scan(struct work_struct *work)
{
	struct tequila_keypad *keypad =
		container_of(work, struct tequila_keypad, work.work);
	struct input_dev *input_dev = keypad->input_dev;
	const struct tequila_keypad_platform_data *pdata = keypad->pdata;
	uint32_t new_state[TEQUILA_MAX_COLS];
	int row, col, code;

	/* de-activate all columns for scanning */
	activate_all_cols(pdata, false);

	memset(new_state, 0, sizeof(new_state));

	/* assert each column and read the row status out */
	for (col = 0; col < pdata->num_col_gpios; col++) {

		activate_col(pdata, col, true);

		for (row = 0; row < pdata->num_row_gpios; row++)
			new_state[col] |=
				row_asserted(pdata, row) ? (1 << row) : 0;

		activate_col(pdata, col, false);
	}

	for (col = 0; col < pdata->num_col_gpios; col++) {
		uint32_t bits_changed;

		bits_changed = keypad->last_key_state[col] ^ new_state[col];
		if (bits_changed == 0)
			continue;

		for (row = 0; row < pdata->num_row_gpios; row++) {
			if ((bits_changed & (1 << row)) == 0)
				continue;

			code = TEQUILA_SCAN_CODE(row, col, keypad->row_shift);
			report_event(input_dev, EV_MSC, MSC_SCAN, code);
			report_event(input_dev, EV_KEY,  keypad->keycodes[code], !!(new_state[col] & (1 << row)));
		}
	}
	input_sync(input_dev);

	memcpy(keypad->last_key_state, new_state, sizeof(new_state));

	activate_all_cols(pdata, true);

	/* Enable IRQs again */
	spin_lock_irq(&keypad->lock);
	keypad->scan_pending = false;
	enable_row_irqs(keypad);
	spin_unlock_irq(&keypad->lock);
}

static irqreturn_t tequila_keypad_interrupt(int irq, void *id)
{
	struct tequila_keypad *keypad = id;
	unsigned long flags;

	spin_lock_irqsave(&keypad->lock, flags);

	/*
	 * See if another IRQ beaten us to it and scheduled the
	 * scan already. In that case we should not try to
	 * disable IRQs again.
	 */
	if (unlikely(keypad->scan_pending || keypad->stopped))
		goto out;

	disable_row_irqs(keypad);
	keypad->scan_pending = true;
	schedule_delayed_work(&keypad->work,
		msecs_to_jiffies(keypad->pdata->debounce_ms));

out:
	spin_unlock_irqrestore(&keypad->lock, flags);
	return IRQ_HANDLED;
}

static int tequila_keypad_start(struct input_dev *dev)
{
	struct tequila_keypad *keypad = input_get_drvdata(dev);

	keypad->stopped = false;
	mb();

	/*
	 * Schedule an immediate key scan to capture current key state;
	 * columns will be activated and IRQs be enabled after the scan.
	 */
	schedule_delayed_work(&keypad->work, 0);

	return 0;
}

static void tequila_keypad_stop(struct input_dev *dev)
{
	struct tequila_keypad *keypad = input_get_drvdata(dev);

	keypad->stopped = true;
	mb();
	flush_work(&keypad->work.work);
	/*
	 * tequila_keypad_scan() will leave IRQs enabled;
	 * we should disable them now.
	 */
	disable_row_irqs(keypad);
}

#ifdef CONFIG_PM
static void tequila_keypad_enable_wakeup(struct tequila_keypad *keypad)
{
	const struct tequila_keypad_platform_data *pdata = keypad->pdata;
	unsigned int gpio;
	int i;

	if (pdata->clustered_irq > 0) {
		if (enable_irq_wake(pdata->clustered_irq) == 0)
			keypad->gpio_all_disabled = true;
	} else {

		for (i = 0; i < pdata->num_row_gpios; i++) {
			if (!test_bit(i, keypad->disabled_gpios)) {
				gpio = pdata->row_gpios[i];

				if (enable_irq_wake(gpio_to_irq(gpio)) == 0)
					__set_bit(i, keypad->disabled_gpios);
			}
		}
	}
}

static void tequila_keypad_disable_wakeup(struct tequila_keypad *keypad)
{
	const struct tequila_keypad_platform_data *pdata = keypad->pdata;	
	unsigned int gpio;
	int i;

	if (pdata->clustered_irq > 0) {
		if (keypad->gpio_all_disabled) {
			disable_irq_wake(pdata->clustered_irq);
			keypad->gpio_all_disabled = false;
		}
	} else {
		for (i = 0; i < pdata->num_row_gpios; i++) {
			if (test_and_clear_bit(i, keypad->disabled_gpios)) {
				gpio = pdata->row_gpios[i];
				disable_irq_wake(gpio_to_irq(gpio));
			}
		}
	}
}

static int tequila_keypad_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct tequila_keypad *keypad = platform_get_drvdata(pdev);

	tequila_keypad_stop(keypad->input_dev);

	if (device_may_wakeup(&pdev->dev))
		tequila_keypad_enable_wakeup(keypad);

	return 0;
}

static int tequila_keypad_resume(struct platform_device *pdev)
{
	struct tequila_keypad *keypad = platform_get_drvdata(pdev);

	if (device_may_wakeup(&pdev->dev))
		tequila_keypad_disable_wakeup(keypad);

	tequila_keypad_start(keypad->input_dev);

	return 0;
}

#endif

extern void mx50_tequila_set_pad_keys(void);

static int __devinit init_tequila_gpio(struct platform_device *pdev,
					struct tequila_keypad *keypad)
{
	const struct tequila_keypad_platform_data *pdata = keypad->pdata;
	int i, err = -EINVAL;

	/* initialized strobe lines as outputs, activated */
	for (i = 0; i < pdata->num_col_gpios; i++) {

		if (pdata->col_gpios[i] < 0)
			break;

		err = gpio_request(pdata->col_gpios[i], "tequila_kbd_col");
		if (err) {
			dev_err(&pdev->dev,
				"failed to request GPIO%d for COL%d\n",
				pdata->col_gpios[i], i);
			goto err_free_cols;
		}

		gpio_direction_output(pdata->col_gpios[i], !pdata->active_low);
	}

	for (i = 0; i < pdata->num_row_gpios; i++) {
		err = gpio_request(pdata->row_gpios[i], "tequila_kbd_row");
		if (err) {
			dev_err(&pdev->dev,
				"failed to request GPIO%d for ROW%d\n",
				pdata->row_gpios[i], i);
			goto err_free_rows;
		}
		gpio_direction_input(pdata->row_gpios[i]);
	}

	if (pdata->clustered_irq > 0) {
		err = request_irq(pdata->clustered_irq,
				tequila_keypad_interrupt,
				pdata->clustered_irq_flags,
				"tequila-keypad", keypad);
		if (err) {
			dev_err(&pdev->dev,
				"Unable to acquire clustered interrupt\n");
			goto err_free_rows;
		}
	} else {
		for (i = 0; i < pdata->num_row_gpios; i++) {
			err = request_irq(gpio_to_irq(pdata->row_gpios[i]),
					tequila_keypad_interrupt,
					IRQF_DISABLED |
					IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING,
					"tequila-keypad", keypad);
			if (err) {
				dev_err(&pdev->dev,
					"Unable to acquire interrupt "
					"for GPIO line %i\n",
					pdata->row_gpios[i]);
				goto err_free_irqs;
			}
		}
	}

	/* initialized as disabled - enabled by input->open */
	disable_row_irqs(keypad);

	mx50_tequila_set_pad_keys();

	return 0;

err_free_irqs:
	while (--i >= 0)
		free_irq(gpio_to_irq(pdata->row_gpios[i]), keypad);
	i = pdata->num_row_gpios;
err_free_rows:
	while (--i >= 0)
		gpio_free(pdata->row_gpios[i]);
	i = pdata->num_col_gpios;
err_free_cols:
	while (--i >= 0)
		gpio_free(pdata->col_gpios[i]);

	return err;
}

static int __devinit tequila_keypad_probe(struct platform_device *pdev)
{
	const struct tequila_keypad_platform_data *pdata;
	const struct tequila_keymap_data *keymap_data;
	struct tequila_keypad *keypad;
	struct input_dev *input_dev;
	unsigned short *keycodes;
	unsigned int row_shift;
	int err;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "no platform data defined\n");
		return -EINVAL;
	}

	keymap_data = pdata->keymap_data;
	if (!keymap_data) {
		dev_err(&pdev->dev, "no keymap data defined\n");
		return -EINVAL;
	}

	row_shift = get_count_order(pdata->num_col_gpios);

	keypad = kzalloc(sizeof(struct tequila_keypad), GFP_KERNEL);
	keycodes = kzalloc((pdata->num_row_gpios << row_shift) *
				sizeof(*keycodes),
			   GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!keypad || !keycodes || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	teqkb_dev = input_dev;
	keypad->input_dev = input_dev;
	keypad->pdata = pdata;
	keypad->keycodes = keycodes;
	keypad->row_shift = row_shift;
	keypad->stopped = true;
	INIT_DELAYED_WORK(&keypad->work, tequila_keypad_scan);
	spin_lock_init(&keypad->lock);

	input_dev->name		= pdev->name;
	input_dev->id.bustype	= BUS_HOST;
	input_dev->dev.parent	= &pdev->dev;
	input_dev->evbit[0]	= BIT_MASK(EV_KEY);
	if (!pdata->no_autorepeat)
		input_dev->evbit[0] |= BIT_MASK(EV_REP);
	input_dev->open		= tequila_keypad_start;
	input_dev->close	= tequila_keypad_stop;

	input_dev->keycode	= keycodes;
	input_dev->keycodesize	= sizeof(*keycodes);
	input_dev->keycodemax	= pdata->num_row_gpios << row_shift;

	tequila_keypad_build_keymap(keymap_data, row_shift,
				   input_dev->keycode, input_dev->keybit);

	input_set_capability(input_dev, EV_MSC, MSC_SCAN);
	input_set_drvdata(input_dev, keypad);

	err = init_tequila_gpio(pdev, keypad);
	if (err)
		goto err_free_mem;

	err = input_register_device(keypad->input_dev);
	if (err)
		goto err_free_mem;

	device_init_wakeup(&pdev->dev, pdata->wakeup);
	platform_set_drvdata(pdev, keypad);

  	/* Create proc entry */
	proc_entry = create_proc_entry( "keypad", 0644, NULL );
    	if (proc_entry == NULL) {
      		dev_err(&pdev->dev, "create_proc: Keyboard could not create proc entry\n");
      		return -ENOMEM;
    	}
	else {
      		proc_entry->read_proc = keypad_proc_read;
      		proc_entry->write_proc = keypad_proc_write;
    	}
	/*end proc entry*/
	return 0;

err_free_mem:
	input_free_device(input_dev);
	kfree(keycodes);
	kfree(keypad);
	return err;
}

static int __devexit tequila_keypad_remove(struct platform_device *pdev)
{
	struct tequila_keypad *keypad = platform_get_drvdata(pdev);
	const struct tequila_keypad_platform_data *pdata = keypad->pdata;
	int i;

	device_init_wakeup(&pdev->dev, 0);

	if (pdata->clustered_irq > 0) {
		free_irq(pdata->clustered_irq, keypad);
	} else {
		for (i = 0; i < pdata->num_row_gpios; i++)
			free_irq(gpio_to_irq(pdata->row_gpios[i]), keypad);
	}

	for (i = 0; i < pdata->num_row_gpios; i++)
		gpio_free(pdata->row_gpios[i]);

	for (i = 0; i < pdata->num_col_gpios; i++)
		gpio_free(pdata->col_gpios[i]);

	input_unregister_device(keypad->input_dev);
	platform_set_drvdata(pdev, NULL);
	kfree(keypad->keycodes);
	kfree(keypad);

	remove_proc_entry("keypad", NULL);

	return 0;
}

static struct platform_driver tequila_keypad_driver = {
	.probe		= tequila_keypad_probe,
	.remove		= __devexit_p(tequila_keypad_remove),
#ifdef CONFIG_PM
	.suspend	= tequila_keypad_suspend,
	.resume		= tequila_keypad_resume,
#endif
	.driver		= {
		.name	= "tequila-keypad",
		.owner	= THIS_MODULE,
	},
};

static int __init tequila_keypad_init(void)
{
	return platform_driver_register(&tequila_keypad_driver);
}

static void __exit tequila_keypad_exit(void)
{
	platform_driver_unregister(&tequila_keypad_driver);
}

module_init(tequila_keypad_init);
module_exit(tequila_keypad_exit);

MODULE_AUTHOR("Manish Lachwani <lachwani@amazon.com>");
MODULE_DESCRIPTION("GPIO Driven Tequila Keypad Driver");
MODULE_ALIAS("platform:tequila-keypad");
MODULE_LICENSE("GPL");

