/*
 * GPIO Multiplexer driver
 *
 * Copyright (C) 2015 Ben Gamari
 * Author:	Ben Gamari <ben@smart-cactus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

struct gpio_mux {
	struct gpio_chip chip;
	struct mutex lock;
	int active;    /* index of active pin or -1 if none */
	int master;    /* GPIO number of master pin */
	int n_selects; /* number of select pins */
	int *selects;  /* GPIO numbers of select pins */
};

static int gpio_mux_get_direction(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static int gpio_mux_set_direction_input(struct gpio_chip *chip, unsigned offset)
{
	return -ENOSYS;
}

static int gpio_mux_set_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	return 0;
}

static int gpio_mux_get(struct gpio_chip *chip, unsigned offset)
{
	struct gpio_mux *mux = (struct gpio_mux *) chip;
	return offset == mux->active;
}

void gpio_mux_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct gpio_mux *mux = (struct gpio_mux *) chip;
	int i;
	mutex_lock(&mux->lock);
	if (value) {
		gpio_set_value(mux->master, 0);
		if (offset != mux->active) {
			for (i = 0; i < mux->n_selects; i++) {
				gpio_set_value(mux->master, 1 & (offset >> (i - mux->n_selects)));
			}
			mux->active = offset;
		}
		gpio_set_value(mux->master, 1);
	} else {
		gpio_set_value(mux->master, 0);
	}
	mutex_unlock(&mux->lock);
}

static const struct of_device_id gpio_mux_match[];

static int gpio_mux_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	const struct of_device_id *match;
	const struct gpio_mux_platform_data *pdata;
	struct resource *res;
	struct gpio_mux *mux;
	int ret, n_gpios;

	match = of_match_device(of_match_ptr(gpio_mux_match), dev);

	pdata = match ? match->data : dev_get_platdata(dev);
	if (!pdata)
		return -EINVAL;

	ret = of_property_read_u32(chip_np, "#gpio-cells", &tmp);
	if (ret)
		return ERR_PTR(ret);

	mux = devm_kzalloc(dev, sizeof(struct gpio_bank) + n_gpios, GFP_KERNEL);
	if (!mux) {
		dev_err(dev, "Memory alloc failed\n");
		return -ENOMEM;
	}

	mutex_init(&mux->lock);

	for (int i=0; i < n_gpios; i++) {
		of_get_named_gpio_flags(node, name, index, &of_flags);
	}

	mux->gpio_chip.label = "GPIO multiplexer";
	mux->gpio_chip.direction_output = gpio_mux_direction_output;
	mux->gpio_chip.direction_input = gpio_mux_direction_input;
	mux->gpio_chip.get = gpio_mux_get_value;
	mux->gpio_chip.set = gpio_mux_set_value;
	mux->gpio_chip.base = -1;

	ret = gpiochip_add(&mux->gpio_chip);
	if (ret) {
		printk(KERN_ERR "Failed to register ADC mux: %d\n", ret);
		return ret;
	}
	return 0;
}

static const struct of_device_id gpio_mux_match[] = {
	{ .compatible = "gpio,multiplexed", },
	{ },
};

static struct platform_driver gpio_mux_driver = {
	.probe		= gpio_mux_probe,
	.driver		= {
		.name	= "gpio_mux",
		.pm	= &gpio_pm_ops,
		.of_match_table = of_match_ptr(gpio_mux_match),
	},
};
