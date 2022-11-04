#include <stdio.h>
#include <errno.h>

#include "pyra_vol_mon.h"

int read_value_and_update_thresholds(
		struct pyra_volume_config *config,
		struct pyra_iio_event_handle *iio)
{
	int ret;
	int value;
	int threshold;

	ret = pyra_iio_get_value(iio);
	if (ret < 0) {
		fprintf(stderr, "Error reading current value: %d\n", ret);
		return -EAGAIN;
	}

	value = ret;

	/* update upper threshold */
	threshold = value + config->step;
	if (threshold > config->max)
		threshold = config->max;

	if (threshold > value) {
		ret = pyra_iio_enable_upper_threshold(iio, threshold);
		if (ret)
			fprintf(stderr, "Failed to enable upper threshold: %d\n", ret);
	}
	else {
		ret = pyra_iio_disable_upper_threshold(iio);
		if (ret < 0)
			fprintf(stderr, "Failed to disable upper threshold: %d\n", ret);
	}

	/* update lower threshold */
	threshold = value - config->step;
	if (threshold < (int)config->min)
		threshold = config->min;

	/* avoid enabling a threshold that's below the step value */
	if (threshold < value && threshold > config->step) {
		ret = pyra_iio_enable_lower_threshold(iio, threshold);
		if (ret < 0)
			fprintf(stderr, "Failed to enable lower threshold: %d\n", ret);
	}
	else {
		ret = pyra_iio_disable_lower_threshold(iio);
		if (ret < 0)
			fprintf(stderr, "Failed to disable lower threshold: %d\n", ret);
	}

	return value;
}