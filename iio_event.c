#include "iio_event.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/iio/events.h>
#include <sys/ioctl.h>

#include "iio_utils.h"

#define PYRA_DEVICE_NAME	"palmas-gpadc"

static int open_event_fd(int dev_num)
{
	int ret;
	int fd, event_fd;
	char *chrdev_name;

	ret = asprintf(&chrdev_name, "/dev/iio:device%d", dev_num);
	if (ret < 0)
		return -ENOMEM;

	fd = open(chrdev_name, 0);
	if (fd == -1) {
		ret = -errno;
		fprintf(stderr, "Failed to open %s\n", chrdev_name);
		goto out_free;
	}

	ret = ioctl(fd, IIO_GET_EVENT_FD_IOCTL, &event_fd);
	if (ret == -1 || event_fd == -1) {
		ret = -errno;
		if (ret == -ENODEV)
			fprintf(stderr,
				"This device does not support events\n");
		else
			fprintf(stderr, "Failed to retrieve event fd\n");
		if (close(fd) == -1)
			perror("Failed to close character device file");

		goto out_free;
	}

	if (close(fd) == -1)  {
		ret = -errno;
		if (close(event_fd) == -1)
			perror("Failed to close event file descriptor");
		goto out_free;
	}

	ret = event_fd;

out_free:
	free(chrdev_name);
	return ret;
}

int pyra_iio_event_open(struct pyra_iio_event_handle* handle, int channel)
{
	const char* device_name = PYRA_DEVICE_NAME;
	char* dev_dir_name = NULL;
	char* input_file = NULL;
	char* lower_threshold_value = NULL;
	char* lower_threshold_enable = NULL;
	char* upper_threshold_value = NULL;
	char* upper_threshold_enable = NULL;
	int dev_num;
	int ret;

	dev_num = find_type_by_name(device_name, "iio:device");
	if (dev_num < 0) {
		printf("Could not find IIO device with name %s\n", device_name);
		return -ENODEV;
	}

	printf("Found IIO device with name %s with device number %d\n",
		device_name, dev_num);

	ret = asprintf(&dev_dir_name, "%siio:device%d", iio_dir, dev_num);
	if (ret < 0)
		return ret;

	ret = asprintf(&input_file, "in_voltage%d_input", channel);
	if (ret < 0)
		goto error_free;

	ret = asprintf(&lower_threshold_enable,
			"events/in_voltage%d_thresh_falling_en", channel);
	if (ret < 0)
		goto error_free;

	ret = asprintf(&lower_threshold_value,
			"events/in_voltage%d_thresh_falling_value", channel);
	if (ret < 0)
		goto error_free;

	ret = asprintf(&upper_threshold_enable,
			"events/in_voltage%d_thresh_rising_en", channel);
	if (ret < 0)
		goto error_free;

	ret = asprintf(&upper_threshold_value,
			"events/in_voltage%d_thresh_rising_value", channel);
	if (ret < 0)
		goto error_free;

	ret = open_event_fd(dev_num);
	if (ret < 0)
		goto error_free;

	handle->dev_dir_name = dev_dir_name;
	handle->input = input_file;
	handle->upper_enable = upper_threshold_enable;
	handle->upper_threshold = upper_threshold_value;
	handle->lower_enable = lower_threshold_enable;
	handle->lower_threshold = lower_threshold_value;

	return ret;
error_free:
	free(upper_threshold_value);
	free(upper_threshold_enable);
	free(lower_threshold_value);
	free(lower_threshold_enable);
	free(input_file);
	free(dev_dir_name);
	return ret;
}

void pyra_iio_event_free(struct pyra_iio_event_handle* handle)
{
	free((char*)handle->lower_threshold);
	free((char*)handle->lower_enable);
	free((char*)handle->upper_threshold);
	free((char*)handle->upper_enable);
	free((char*)handle->input);
	free((char*)handle->dev_dir_name);
}