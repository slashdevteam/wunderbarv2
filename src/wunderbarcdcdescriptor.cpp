#include "devicedescriptor.h"
#include "usb_device_config.h"

namespace wunderbar
{

device_specific_descriptors cdcDescriptors =
{
    // deviceDescriptor
    {
        /* Size of this descriptor in bytes */
        USB_DESCRIPTOR_LENGTH_DEVICE,
        /* DEVICE Descriptor Type */
        USB_DESCRIPTOR_TYPE_DEVICE,
        /* USB Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H). */
        USB_SHORT_GET_LOW(USB_DEVICE_SPECIFIC_BCD_VERSION), USB_SHORT_GET_HIGH(USB_DEVICE_SPECIFIC_BCD_VERSION),
        /* Class code (assigned by the USB-IF). */
        USB_DEVICE_CLASS,
        /* Subclass code (assigned by the USB-IF). */
        USB_DEVICE_SUBCLASS,
        /* Protocol code (assigned by the USB-IF). */
        USB_DEVICE_PROTOCOL,
        /* Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid) */
        USB_CONTROL_MAX_PACKET_SIZE,
        /* Vendor ID (assigned by the USB-IF) */
        0xA2,0x15,
        /* Product ID (assigned by the manufacturer) */
        0x00, 0x02,
        /* Device release number in binary-coded decimal */
        USB_SHORT_GET_LOW(USB_DEVICE_DEMO_BCD_VERSION), USB_SHORT_GET_HIGH(USB_DEVICE_DEMO_BCD_VERSION),
        /* Index of string descriptor describing manufacturer */
        0x01,
        /* Index of string descriptor describing product */
        0x02,
        /* Index of string descriptor describing the device's serial number */
        0x00,
        /* Number of possible configurations */
        USB_DEVICE_CONFIGURATION_COUNT,
    },
    // configurationDescriptor
    {/* Size of this descriptor in bytes */
    USB_DESCRIPTOR_LENGTH_CONFIGURE,
    /* CONFIGURATION Descriptor Type */
    USB_DESCRIPTOR_TYPE_CONFIGURE,
    /* Total length of data returned for this configuration. */
    USB_SHORT_GET_LOW(USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL),
    USB_SHORT_GET_HIGH(USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL),
    /* Number of interfaces supported by this configuration */
    USB_CDC_VCOM_INTERFACE_COUNT,
    /* Value to use as an argument to the SetConfiguration() request to select this configuration */
    USB_CDC_VCOM_CONFIGURE_INDEX,
    /* Index of string descriptor describing this configuration */
    0,
    /* Configuration characteristics D7: Reserved (set to one) D6: Self-powered D5: Remote Wakeup D4...0: Reserved
       (reset to zero) */
    (USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_D7_MASK) |
        (USB_DEVICE_CONFIG_SELF_POWER << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_SELF_POWERED_SHIFT) |
        (USB_DEVICE_CONFIG_REMOTE_WAKEUP << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_REMOTE_WAKEUP_SHIFT),
    /* Maximum power consumption of the USB * device from the bus in this specific * configuration when the device is
       fully * operational. Expressed in 2 mA units *  (i.e., 50 = 100 mA).  */
    USB_DEVICE_MAX_POWER,

    /* Communication Interface Descriptor */
    USB_DESCRIPTOR_LENGTH_INTERFACE, USB_DESCRIPTOR_TYPE_INTERFACE, USB_CDC_VCOM_COMM_INTERFACE_INDEX, 0x00,
    USB_CDC_VCOM_ENDPOINT_CIC_COUNT, USB_CDC_VCOM_CIC_CLASS, USB_CDC_VCOM_CIC_SUBCLASS, USB_CDC_VCOM_CIC_PROTOCOL,
    0x00, /* Interface Description String Index*/

    /* CDC Class-Specific descriptor */
    USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC, /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE,  /* CS_INTERFACE Descriptor Type */
    USB_CDC_HEADER_FUNC_DESC, 0x10,
    0x01, /* USB Class Definitions for Communications the Communication specification version 1.10 */

    USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG, /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE, /* CS_INTERFACE Descriptor Type */
    USB_CDC_CALL_MANAGEMENT_FUNC_DESC,
    0x01, /*Bit 0: Whether device handle call management itself 1, Bit 1: Whether device can send/receive call
             management information over a Data Class Interface 0 */
    0x01, /* Indicates multiplexed commands are handled via data interface */

    USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT,   /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE, /* CS_INTERFACE Descriptor Type */
    USB_CDC_ABSTRACT_CONTROL_FUNC_DESC,
    0x06, /* Bit 0: Whether device supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and
             Get_Comm_Feature 0, Bit 1: Whether device supports the request combination of Set_Line_Coding,
             Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State 1, Bit ...  */

    USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC, /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE, /* CS_INTERFACE Descriptor Type */
    USB_CDC_UNION_FUNC_DESC, 0x00,        /* The interface number of the Communications or Data Class interface  */
    0x01,                                 /* Interface number of subordinate interface in the Union  */

    /*Notification Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT | (USB_IN << 7U),
    USB_ENDPOINT_INTERRUPT, USB_SHORT_GET_LOW(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE),
    USB_SHORT_GET_HIGH(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE), FS_CDC_VCOM_INTERRUPT_IN_INTERVAL,

    /* Data Interface Descriptor */
    USB_DESCRIPTOR_LENGTH_INTERFACE, USB_DESCRIPTOR_TYPE_INTERFACE, USB_CDC_VCOM_DATA_INTERFACE_INDEX, 0x00,
    USB_CDC_VCOM_ENDPOINT_DIC_COUNT, USB_CDC_VCOM_DIC_CLASS, USB_CDC_VCOM_DIC_SUBCLASS, USB_CDC_VCOM_DIC_PROTOCOL,
    0x00, /* Interface Description String Index*/

    /*Bulk IN Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, USB_CDC_VCOM_BULK_IN_ENDPOINT | (USB_IN << 7U),
    USB_ENDPOINT_BULK, USB_SHORT_GET_LOW(FS_CDC_VCOM_BULK_IN_PACKET_SIZE),
    USB_SHORT_GET_HIGH(FS_CDC_VCOM_BULK_IN_PACKET_SIZE), 0x00, /* The polling interval value is every 0 Frames */

    /*Bulk OUT Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, USB_CDC_VCOM_BULK_OUT_ENDPOINT | (USB_OUT << 7U),
    USB_ENDPOINT_BULK, USB_SHORT_GET_LOW(FS_CDC_VCOM_BULK_OUT_PACKET_SIZE),
    USB_SHORT_GET_HIGH(FS_CDC_VCOM_BULK_OUT_PACKET_SIZE), 0x00, /* The polling interval value is every 0 Frames */},
    // languageString
    {sizeof(cdcDescriptors.languageString), USB_DESCRIPTOR_TYPE_STRING, 0x09, 0x04},
    // manufacturerString
    {
    sizeof(cdcDescriptors.manufacturerString),
    USB_DESCRIPTOR_TYPE_STRING,
    'C',
    0x00U,
    'O',
    0x00U,
    'N',
    0x00U,
    'R',
    0x00U,
    'A',
    0x00U,
    'D',
    0x00U,
    ' ',
    0x00U,
    'E',
    0x00U,
    'L',
    0x00U,
    'E',
    0x00U,
    'C',
    0x00U,
    'T',
    0x00U,
    'R',
    0x00U,
    'O',
    0x00U,
    'N',
    0x00U,
    'I',
    0x00U,
    'C',
    0x00U,
    },
    // productString
    {sizeof(cdcDescriptors.productString),
                 USB_DESCRIPTOR_TYPE_STRING,
                 'W',
                 0,
                 'U',
                 0,
                 'N',
                 0,
                 'D',
                 0,
                 'E',
                 0,
                 'R',
                 0,
                 'B',
                 0,
                 'A',
                 0,
                 'R',
                 0,
                 ' ',
                 0,
                 'V',
                 0,
                 '2',
                 0,
                 ' ',
                 0,
                 'C',
                 0,
                 'D',
                 0,
                 'C',
                 0,
                 'V',
                 0,
                 'C',
                 0,
                 'O',
                 0,
                 'M',
                 0},
    // stringDescriptorArray
    {cdcDescriptors.languageString,
                          cdcDescriptors.manufacturerString,
                          cdcDescriptors.productString},
    // stringDescriptorLength
    {sizeof(cdcDescriptors.languageString),
                           sizeof(cdcDescriptors.manufacturerString),
                           sizeof(cdcDescriptors.productString)},
    // deviceLanguage
    {{cdcDescriptors.stringDescriptorArray,
                    cdcDescriptors.stringDescriptorLength,
                    (uint16_t)0x0409}}
};

}
