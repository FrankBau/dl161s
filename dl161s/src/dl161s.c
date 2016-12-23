#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <usb.h>
#include <time.h>
#include <locale.h>

#define VID 0x10c4 /* Cygnal Integrated Products, Inc. */
#define PID 0xea61 /* Cygnal Integrated Products, Inc. CP210x UART Bridge */

int EP_IN = 0;
int EP_OUT = 0;

#define BUFSIZE 64 /* wMaxPacketSize = 1x 64 bytes */
#define TIMEOUT 5000

#define INVALID_CALIBRATION_VALUE (127)

struct usb_device *open_vid_pid(uint16_t vid, uint16_t pid)
{
	int ret;
	struct usb_bus *busses;
	struct usb_bus *bus_cur;
	struct usb_device *dev_cur;
	struct usb_device *dev = NULL;

	ret = usb_find_busses();
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_find_busses failed with status %i: %s\n", ret, usb_strerror());
		return NULL;
	}

	ret = usb_find_devices();
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_find_devices failed with status %i: %s\n", ret, usb_strerror());
		return NULL;
	}

	busses = usb_get_busses();

	// find dev
	for (bus_cur = busses; bus_cur != NULL; bus_cur = bus_cur->next)
	{
		for (dev_cur = bus_cur->devices; dev_cur != NULL; dev_cur = dev_cur->next)
		{
			if (dev_cur->descriptor.idVendor == vid && dev_cur->descriptor.idProduct == pid)
			{
				dev = dev_cur;
				break;
			}
		}
		if (dev != NULL)
		{
			break;
		}
	}
	if (dev == NULL)
	{
		syslog(LOG_ERR, "device %04x:%04x not found\n", VID, PID);
		return NULL;
	}

	return dev;
}

void print_endpoint(struct usb_endpoint_descriptor *endpoint)
{
    printf("=====End point Information====\n");
    printf("bEndpointAddress: %x\n", endpoint->bEndpointAddress);
    printf("bmAttributes:     %x\n", endpoint->bmAttributes);
    printf("wMaxPacketSize:   %d\n", endpoint->wMaxPacketSize);
    printf("bInterval:        %d\n", endpoint->bInterval);
    printf("bRefresh:         %d\n", endpoint->bRefresh);
    printf("bSynchAddress:    %d\n", endpoint->bSynchAddress);
}

void print_altsetting(struct usb_interface_descriptor *interface)
{
    int i;

    printf("\n=====Alternate Setting Information====\n");
    printf("bInterfaceNumber:   %d\n", interface->bInterfaceNumber);
    printf("bAlternateSetting:  %d\n", interface->bAlternateSetting);
    printf("bNumEndpoints:      %d\n", interface->bNumEndpoints);
    printf("bInterfaceClass:    %d\n", interface->bInterfaceClass);
    printf("bInterfaceSubClass: %d\n", interface->bInterfaceSubClass);
    printf("bInterfaceProtocol: %d\n", interface->bInterfaceProtocol);
    printf("iInterface:         %d\n", interface->iInterface);

    for (i = 0; i < interface->bNumEndpoints; i++)
        print_endpoint(&interface->endpoint[i]);
}

void print_interface(struct usb_interface *interface)
{
    int i;

    for (i = 0; i < interface->num_altsetting; i++)
        print_altsetting(&interface->altsetting[i]);
}

void print_configuration(struct usb_config_descriptor *config)
{
    int i;

    printf("=====Configuration Information====\n");
    printf("wTotalLength:         %d\n", config->wTotalLength);
    printf("bNumInterfaces:       %d\n", config->bNumInterfaces);
    printf("bConfigurationValue:  %d\n", config->bConfigurationValue);
    printf("iConfiguration:       %d\n", config->iConfiguration);
    printf("bmAttributes:         %x\n", config->bmAttributes);
    printf("MaxPower:             %d\n", config->MaxPower);

    for (i = 0; i < config->bNumInterfaces; i++)
        print_interface(&config->interface[i]);
}

int print_device(struct usb_device *dev, struct usb_dev_handle *udev)
{
    char str[100];
    int ret, i;
    {
        if (dev->descriptor.iManufacturer) {
            ret = usb_get_string_simple(udev, dev->descriptor.iManufacturer, str, sizeof(str));
            if (ret > 0)
            {
                printf("Manufacturer is %s\n",str);
            }
        }
        if (dev->descriptor.iProduct) {
            ret = usb_get_string_simple(udev, dev->descriptor.iProduct, str, sizeof(str));
            if (ret > 0)
            {
                printf("Product is %s\n",str);
            }
        } 

    }

    printf("Possible configurations are %x\n",dev->descriptor.bNumConfigurations);

    for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
        print_configuration(&dev->config[i]);

    return 0;
}

// see http://www.produktinfo.conrad.com/datenblaetter/100000-124999/100032-da-01-en-Schnittstelle_DL160S_SCHALLPDATENLOGGER.pdf
// request one live measurment dBA fast peak per second
char setup64[64] = {
	0x0a, 	// green LED blink interval: 10 seconds
	(0<<7) |	// 0/1: auto/manual
	(1<<6) |	// 0/1: realtime/store
	(0<<5) |	// 0/1: check?
	(1<<4) |	// 0/1: normal/peak
	(1<<3) |	// 0/1: dBC/dBA (frequency weighting)
	(0<<2) |	// 0/1: slow/fast (time weighting)
	(1<<0) ,	// 0/1/2/3: n.a. / sample rate in seconds / sample rate in minutes / sample rate in hours
	0x01, // sample rate: 1
	0x3c, // alarm LED hi level dB (set to 60 dB to give some visual feedback)
	0x1e, // alarm LED lo level dB
	0x10, // year (2-digit): 2016
	0x0b, // month: 11
	0x06, // day: 6
	0x10, // hour: 16
	0x08, // minute: 8
	0x0b, // seconds: 11
	0x01, // #samples (3U8) MSB 
	0xfb, // #samples (3U8) ...
	0x80, // #samples (3U8) LSB 0x01fb80 = 129920 samples
	0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x39, 0xbb, 0x78, 0x03, 0x00, 0x00, 0x00,
	0x0e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void print_buffer( char *buf, int len, FILE *file )
{
	int i;

	for (i = 0; i < len; i++)
	{
		if (i % 8 == 0)
			fprintf(file,"\n\t");
		fprintf(file," %02x", buf[i] & 0xFF);
	}
	fprintf(file,"\n");
}

// calibration value is a signed int in 0.1 dB steps, 0 is 0.0 dB, range according to manual: +/- 12.0 dB
// calibration value is a relative value which is added to the current calibration value stored in the device
// the accumulated value is returned from the device
// i.e. sending a 0.0 retrieves the stored value without altering calibration
int send_calibration(struct usb_dev_handle *dev_hdl, int8_t value)
{
	char buf[BUFSIZE];
	int ret;

	syslog(LOG_DEBUG, "send_calibration(%i)\n", value );
	// send calibration
	buf[0] = 0x0C;
	buf[1] = value;
	buf[2] = 0x00;

	ret = usb_bulk_write(
		dev_hdl,
		EP_OUT,
		buf,
		3,
		TIMEOUT
	);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_bulk_write failed with status %i: %s\n", ret, usb_strerror());
		return INVALID_CALIBRATION_VALUE;
	}

	ret = usb_bulk_read(
		dev_hdl,
		EP_IN,
		buf,
		64,
		TIMEOUT
	);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_bulk_read failed with status %i: %s\n", ret, usb_strerror());
		return INVALID_CALIBRATION_VALUE;
	}

	// response is the accumulated absolute calibration value
	if(ret==1) {
		int accumulated_value = (int8_t)buf[0];
		syslog(LOG_DEBUG, "send_calibration returns: %i\n", accumulated_value );
		return accumulated_value;
	}
	else {
		syslog(LOG_ERR, "unexpected calibration response: %i bytes: %i ...\n", ret, (int8_t)buf[0] );
	}

	return INVALID_CALIBRATION_VALUE;
}

int main (int argc, char **argv)
{
	char buf[BUFSIZE];
	int ret;
	struct usb_device *dev;
	struct usb_dev_handle *dev_hdl = NULL;

	int last_day = -1;	// when day changes, a new file will be used
	FILE *data_file = NULL;	// measurment data file stream

	int c;
	int calibration_value = INVALID_CALIBRATION_VALUE;

	openlog(NULL,0,0);

	while ((c = getopt (argc, argv, "c:")) != -1) {
		switch (c) {
#if 0
			case 'c':
				calibration_value = atoi(optarg);
				if( (calibration_value < -125) || (calibration_value > +125) ) {
					fprintf(stderr,"calibration value %i out of range, abort.\n", calibration_value);
					return -1;
				}
				break;
#endif
			case '?':
				fprintf(stderr,"unknown option, abort.\n");
				return -1;
			default:
				abort();
		}
	}
	if( optind < argc ) {
		// trailing non-option arguments
		fprintf(stderr,"trailing non-option arguments, abort.\n");
		return -1;
	}

again:
	syslog(LOG_NOTICE, "started" );

	//usb_set_debug(255);
	usb_init();

	dev = open_vid_pid(VID,PID);
	if(dev == NULL) {
		syslog(LOG_ERR, "open_vid_pid failed, exit\n");
		return -1;
	}

	dev_hdl = usb_open(dev);
	if (dev_hdl == NULL)
	{
		syslog(LOG_ERR, "usb_open failed: %s\n", usb_strerror());
		return -1;
	}

	//print_device(dev, dev_hdl);

	EP_OUT = dev->config[0].interface[0].altsetting[0].endpoint[0].bEndpointAddress;
	EP_IN = dev->config[0].interface[0].altsetting[0].endpoint[1].bEndpointAddress;

	ret = usb_reset(dev_hdl);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_reset failed with status %i: %s\n", ret, usb_strerror());
		return ret;
	}

#if 0
	ret = usb_get_descriptor(
		dev_hdl, 
		2,	// unsigned char type,
	        0x00,	// unsigned char index, 
		buf,	// void *buf, 
		9	// int size
	);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_get_descriptor 9 failed with status %i: %s\n", ret, usb_strerror());
		return ret;
	}

	ret = usb_get_descriptor(
		dev_hdl, 
		2,	// unsigned char type,
	        0x00,	// unsigned char index, 
		buf,	// void *buf, 
		32	// int size
	);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_get_descriptor 32 failed with status %i: %s\n", ret, usb_strerror());
		return ret;
	}
#endif

	ret = usb_set_configuration(dev_hdl, 1); // bConfigurationValue=1, iConfiguration=0
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_set_configuration failed with status %i: %s\n", ret, usb_strerror());
		return ret;
	}

	ret = usb_claim_interface(dev_hdl, 0); // bInterfaceNumber=0, bAlternateSetting=0, bNumEndpoints=2
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_claim_interface failed with status %i: %s\n", ret, usb_strerror());
		return ret;
	}

#if 0
	// usb_control_msg request 0 failed with status -32: error sending control message: Broken pipe
	ret = usb_control_msg(
		dev_hdl, 
		0x40, 	// int requesttype, 
		0,	// int request,
		0xFFFF,	// int value, 
		0,	// int index, 
		NULL,	// char *bytes, 
		0,	// int size, 
		TIMEOUT	// int timeout
	);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_control_msg request 0 failed with status %i: %s\n", ret, usb_strerror());
		// return -1;
	}
#endif
	ret = usb_control_msg(
		dev_hdl, 
		0x40, 	// int requesttype, 
		2,	// int request,
		0x0002,	// int value, 
		0,	// int index, 
		NULL,	// char *bytes, 
		0,	// int size, 
		TIMEOUT	// int timeout
	);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_control_msg request 2 failed with status %i: %s\n", ret, usb_strerror());
		// return -1;	// yes, this might fail, ignore!
	}

	// send setup
	buf[0] = 0x0E;
	buf[1] = 0x40;
	buf[2] = 0x00;

	ret = usb_bulk_write(
		dev_hdl,
		EP_OUT,
		buf,
		3,
		TIMEOUT
	);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_bulk_write send setup 0x0e failed with status %i: %s\n", ret, usb_strerror());
		return ret;
	}

	// send setup struct
	ret = usb_bulk_write(
		dev_hdl,
		EP_OUT,
		setup64,
		64,
		TIMEOUT
	);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_bulk_write send setup (64 bytes) failed with status %i: %s\n", ret, usb_strerror());
		return ret;
	}

	// read response 0xff
	ret = usb_bulk_read(
		dev_hdl,
		EP_IN,
		buf,
		64,
		TIMEOUT
	);
	if (ret < 0)
	{
		syslog(LOG_ERR, "usb_bulk_read failed with status %i: %s\n", ret, usb_strerror());
		return ret;
	}
	if( (ret==1) && (buf[0]==(char)0xFF) ) {
		// ack
	} else {
		syslog(LOG_ERR, "usb_bulk_read: got %i bytes unexpected response\n", ret );
		print_buffer(buf,ret,stderr);
	}

	sleep(5);

#if 0
	// locale support often not installed :-(
	char *rc = setlocale (LC_ALL, "de_DE" );
	if(rc==NULL) {
		syslog(LOG_ERR, "failed to set locale\n");
	}
#endif

	int stored_calibration_value = send_calibration(dev_hdl,0);
	syslog(LOG_NOTICE, "found stored calibration value: %i*0.1 dB\n", stored_calibration_value);

	if(calibration_value != INVALID_CALIBRATION_VALUE) {
		int new_stored_calibration_value = send_calibration(dev_hdl,calibration_value);
		syslog(LOG_NOTICE, "new stored calibration value: %i*0.1 dB\n", new_stored_calibration_value );
	}

	while(1) {

		usleep(1100000);

		// request measurement
		buf[0] = 0xFF;
		buf[1] = 0x00;
		buf[2] = 0x00;

		ret = usb_bulk_write(
			dev_hdl,
			EP_OUT,
			buf,
			3,
			TIMEOUT
		);
		if (ret < 0)
		{
			syslog(LOG_ERR, "usb_bulk_write failed with status %i: %s\n", ret, usb_strerror());
			return ret;
		}

		ret = usb_bulk_read(
			dev_hdl,
			EP_IN,
			buf,
			64,
			TIMEOUT
		);
		if (ret < 0)
		{
			syslog(LOG_ERR, "usb_bulk_read failed with status %i: %s\n", ret, usb_strerror());
			return ret;
		}

		if(ret==2) {
			int x = 256*(uint8_t)buf[1] + (uint8_t)buf[0]; // measurement value in 0.1 dbA

			if(x == 0) {
				// yes, I saw that when sleep(1) was used (too fast?)
				syslog(LOG_ERR, "got result 0.0 from measurement --> reset USB\n");
				usb_close(dev_hdl);
				goto again;
			}

			time_t curtime = time(NULL);
			struct tm *loctime = localtime(&curtime);

			if(last_day != loctime->tm_mday ) {
				if(data_file)
					fclose(data_file);
				char filename[80];
				strftime(filename,sizeof filename,"/var/www/html/logs/%Y-%m-%d.csv", loctime );
				data_file = fopen(filename,"a");
				if(data_file==NULL) {
					syslog(LOG_ERR, "failed to open logfile %s, error %d\n", filename, errno );
					return -1;
				}
				last_day = loctime->tm_mday;
			}
			char timebuf[80];
			char logline[80];
			strftime(timebuf,sizeof timebuf,"%Y-%m-%d %H:%M:%S", loctime );
			// each line is padded to 32 chars.
			// for buffered IO, each buffer (typ. BUFSIZ=4096)
			// will contain an integral number of lines.
			int count = snprintf(logline, sizeof logline, "%s;      %3d.%d\n", timebuf, x/10, x%10 );
			int rc = fwrite( logline, 1, count, data_file );
			if( rc != count ) {
				syslog(LOG_ERR, "failed to write line %s to logfile, error %d\n", logline, errno );
			}
		} else {
			syslog(LOG_ERR, "usb_bulk_read unexpectedly transferred %i bytes", ret);
			print_buffer(buf,ret,stderr);

		}
	}

	if (dev_hdl != NULL)
		usb_close(dev_hdl);

	if(data_file!=NULL)
		fclose(data_file);

	syslog(LOG_NOTICE, "ended");
	closelog();

	return 0;
}
