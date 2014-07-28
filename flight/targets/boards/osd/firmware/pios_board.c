/**
 ******************************************************************************
 *
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Defines board specific static initializers for hardware for the OPOSD board.
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "inc/openpilot.h"
#include <pios_board_info.h>
#include <uavobjectsinit.h>
#include <hwsettings.h>
#include <manualcontrolsettings.h>
#include <gcsreceiver.h>
#include <taskinfo.h>

/*
 * Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "../board_hw_defs.c"

/* Private macro -------------------------------------------------------------*/
#define countof(a) (sizeof(a) / sizeof(*(a)))

/* Private variables ---------------------------------------------------------*/

#if defined(PIOS_INCLUDE_ADC)
#include <pios_adc_priv.h>
void PIOS_ADC_DMC_irq_handler(void);
void DMA2_Stream4_IRQHandler(void) __attribute__((alias("PIOS_ADC_DMC_irq_handler")));
struct pios_adc_cfg pios_adc_cfg = {
    .adc_dev = ADC1,
    .dma     = {
        .irq                                       = {
            .flags = (DMA_FLAG_TCIF4 | DMA_FLAG_TEIF4 | DMA_FLAG_HTIF4),
            .init  = {
                .NVIC_IRQChannel    = DMA2_Stream4_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },
        .rx                                        = {
            .channel = DMA2_Stream4,
            .init    = {
                .DMA_Channel                       = DMA_Channel_0,
                .DMA_PeripheralBaseAddr            = (uint32_t)&ADC1->DR
            },
        }
    },
    .half_flag = DMA_IT_HTIF4,
    .full_flag = DMA_IT_TCIF4,
};
void PIOS_ADC_DMC_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_ADC_DMA_Handler();
}

#endif /* if defined(PIOS_INCLUDE_ADC) */

#define PIOS_COM_TELEM_RF_RX_BUF_LEN  128
#define PIOS_COM_TELEM_RF_TX_BUF_LEN  128

#define PIOS_COM_AUX_RX_BUF_LEN       512
#define PIOS_COM_AUX_TX_BUF_LEN       512

#define PIOS_COM_GPS_RX_BUF_LEN       32

#define PIOS_COM_TELEM_USB_RX_BUF_LEN 65
#define PIOS_COM_TELEM_USB_TX_BUF_LEN 65

#define PIOS_COM_BRIDGE_RX_BUF_LEN    65
#define PIOS_COM_BRIDGE_TX_BUF_LEN    12

uint32_t pios_com_aux_id;
uint32_t pios_com_gps_id;
uint32_t pios_com_telem_usb_id;
uint32_t pios_com_telem_rf_id;

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id = 0;

/**
 * TIM3 is triggered by the HSYNC signal into its ETR line and will divide the
 *  APB1_CLOCK to generate a pixel clock that is used by the SPI CLK lines.
 * TIM4 will be synced to it and will divide by that times the pixel width to
 *  fire an IRQ when the last pixel of the line has been output.  Then the timer will
 *  be rearmed and wait for the next HSYNC signal.
 * The critical timing detail is that the task be _DISABLED_ at the end of the line
 *  before an extra pixel is clocked out
 *  or we will need to configure the DMA task per line
 */
#include "pios_tim_priv.h"

void PIOS_Board_Init(void)
{
    // Delay system
    PIOS_DELAY_Init();

    const struct pios_board_info *bdinfo = &pios_board_info_blob;

    PIOS_LED_Init(&pios_led_cfg);

#if defined(PIOS_INCLUDE_SPI)
    /* Set up the SPI interface to the SD card */
    if (PIOS_SPI_Init(&pios_spi_sdcard_id, &pios_spi_sdcard_cfg)) {
        PIOS_Assert(0);
    }

#if defined(PIOS_INCLUDE_SDCARD)
    /* Enable and mount the SDCard */
    PIOS_SDCARD_Init(pios_spi_sdcard_id);
    PIOS_SDCARD_MountFS(0);
#endif
#endif /* PIOS_INCLUDE_SPI */

#ifdef PIOS_INCLUDE_FLASH_LOGFS_SETTINGS
    uintptr_t flash_id;
    PIOS_Flash_Internal_Init(&flash_id, &flash_internal_cfg);
    PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_cfg, &pios_internal_flash_driver, flash_id);
#elif !defined(PIOS_USE_SETTINGS_ON_SDCARD)
#error No setting storage specified. (define PIOS_USE_SETTINGS_ON_SDCARD or INCLUDE_FLASH_SECTOR_SETTINGS)
#endif

    /* Initialize the task monitor */
    if (PIOS_TASK_MONITOR_Initialize(TASKINFO_RUNNING_NUMELEM)) {
        PIOS_Assert(0);
    }

    /* Initialize the delayed callback library */
    PIOS_CALLBACKSCHEDULER_Initialize();

    /* Initialize UAVObject libraries */
    EventDispatcherInitialize();
    UAVObjInitialize();

    HwSettingsInitialize();

#ifdef PIOS_INCLUDE_WDG
    /* Initialize watchdog as early as possible to catch faults during init */
    PIOS_WDG_Init();
#endif /* PIOS_INCLUDE_WDG */

    /* Initialize the alarms library */
    AlarmsInitialize();

    /* IAP System Setup */
    PIOS_IAP_Init();
    uint16_t boot_count = PIOS_IAP_ReadBootCount();
    if (boot_count < 3) {
        PIOS_IAP_WriteBootCount(++boot_count);
        AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
    } else {
        /* Too many failed boot attempts, force hwsettings to defaults */
        HwSettingsSetDefaults(HwSettingsHandle(), 0);
        AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
    }


#if defined(PIOS_INCLUDE_RTC)
    /* Initialize the real-time clock and its associated tick */
    PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif

#if defined(PIOS_INCLUDE_USB)
    /* Initialize board specific USB data */
    PIOS_USB_BOARD_DATA_Init();

    /* Flags to determine if various USB interfaces are advertised */
    bool usb_hid_present = false;
    bool usb_cdc_present = false;

#if defined(PIOS_INCLUDE_USB_CDC)
    if (PIOS_USB_DESC_HID_CDC_Init()) {
        PIOS_Assert(0);
    }
    usb_hid_present = true;
    usb_cdc_present = true;
#else
    if (PIOS_USB_DESC_HID_ONLY_Init()) {
        PIOS_Assert(0);
    }
    usb_hid_present = true;
#endif

    uint32_t pios_usb_id;
    PIOS_USB_Init(&pios_usb_id, PIOS_BOARD_HW_DEFS_GetUsbCfg(bdinfo->board_rev));

#if defined(PIOS_INCLUDE_USB_CDC)

    uint8_t hwsettings_usb_vcpport;
    /* Configure the USB VCP port */
    HwSettingsUSB_VCPPortGet(&hwsettings_usb_vcpport);

    if (!usb_cdc_present) {
        /* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
        hwsettings_usb_vcpport = HWSETTINGS_USB_VCPPORT_DISABLED;
    }

    switch (hwsettings_usb_vcpport) {
    case HWSETTINGS_USB_VCPPORT_DISABLED:
        break;
    case HWSETTINGS_USB_VCPPORT_USBTELEMETRY:
#if defined(PIOS_INCLUDE_COM)
        {
            uint32_t pios_usb_cdc_id;
            if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
                PIOS_Assert(0);
            }
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_telem_usb_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                              rx_buffer, PIOS_COM_TELEM_USB_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_COM */
        break;
    case HWSETTINGS_USB_VCPPORT_COMBRIDGE:
#if defined(PIOS_INCLUDE_COM)
        {
            uint32_t pios_usb_cdc_id;
            if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
                PIOS_Assert(0);
            }
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_vcp_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                              rx_buffer, PIOS_COM_BRIDGE_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_BRIDGE_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_COM */
        break;
    }
#endif /* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
    /* Configure the usb HID port */
    uint8_t hwsettings_usb_hidport;
    HwSettingsUSB_HIDPortGet(&hwsettings_usb_hidport);

    if (!usb_hid_present) {
        /* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
        hwsettings_usb_hidport = HWSETTINGS_USB_HIDPORT_DISABLED;
    }

    switch (hwsettings_usb_hidport) {
    case HWSETTINGS_USB_HIDPORT_DISABLED:
        break;
    case HWSETTINGS_USB_HIDPORT_USBTELEMETRY:
#if defined(PIOS_INCLUDE_COM)
        {
            uint32_t pios_usb_hid_id;
            if (PIOS_USB_HID_Init(&pios_usb_hid_id, &pios_usb_hid_cfg, pios_usb_id)) {
                PIOS_Assert(0);
            }
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_telem_usb_id, &pios_usb_hid_com_driver, pios_usb_hid_id,
                              rx_buffer, PIOS_COM_TELEM_USB_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_COM */
        break;
    }

#endif /* PIOS_INCLUDE_USB_HID */

    if (usb_hid_present || usb_cdc_present) {
        PIOS_USBHOOK_Activate();
    }
#endif /* PIOS_INCLUDE_USB */

#if defined(PIOS_INCLUDE_COM)
#if defined(PIOS_INCLUDE_GPS)

    uint32_t pios_usart_gps_id;
    if (PIOS_USART_Init(&pios_usart_gps_id, &pios_usart_gps_flexi_io_cfg)) {
        PIOS_Assert(0);
    }

    uint8_t *gps_rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_GPS_RX_BUF_LEN);
    PIOS_Assert(gps_rx_buffer);
    if (PIOS_COM_Init(&pios_com_gps_id, &pios_usart_com_driver, pios_usart_gps_id,
                      gps_rx_buffer, PIOS_COM_GPS_RX_BUF_LEN,
                      NULL, 0)) {
        PIOS_Assert(0);
    }

#endif /* PIOS_INCLUDE_GPS */

#if defined(PIOS_INCLUDE_COM_AUX)
    {
        uint32_t pios_usart_aux_id;

        if (PIOS_USART_Init(&pios_usart_aux_id, &pios_usart_aux_cfg)) {
            PIOS_DEBUG_Assert(0);
        }

        uint8_t *aux_rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_AUX_RX_BUF_LEN);
        uint8_t *aux_tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_AUX_TX_BUF_LEN);
        PIOS_Assert(aux_rx_buffer);
        PIOS_Assert(aux_tx_buffer);

        if (PIOS_COM_Init(&pios_com_aux_id, &pios_usart_com_driver, pios_usart_aux_id,
                          aux_rx_buffer, PIOS_COM_AUX_RX_BUF_LEN,
                          aux_tx_buffer, PIOS_COM_AUX_TX_BUF_LEN)) {
            PIOS_DEBUG_Assert(0);
        }
    }
#else
    pios_com_aux_id = 0;
#endif /* PIOS_INCLUDE_COM_AUX */

#if defined(PIOS_INCLUDE_COM_TELEM)
    { /* Eventually add switch for this port function */
        uint32_t pios_usart_telem_rf_id;
        if (PIOS_USART_Init(&pios_usart_telem_rf_id, &pios_usart_telem_fltctrl_cfg)) {
            PIOS_Assert(0);
        }

        uint8_t *telem_rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_RF_RX_BUF_LEN);
        uint8_t *telem_tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_RF_TX_BUF_LEN);
        PIOS_Assert(telem_rx_buffer);
        PIOS_Assert(telem_tx_buffer);
        if (PIOS_COM_Init(&pios_com_telem_rf_id, &pios_usart_com_driver, pios_usart_telem_rf_id,
                          telem_rx_buffer, PIOS_COM_TELEM_RF_RX_BUF_LEN,
                          telem_tx_buffer, PIOS_COM_TELEM_RF_TX_BUF_LEN)) {
            PIOS_Assert(0);
        }
    }
#else
    pios_com_telem_rf_id = 0;
#endif /* PIOS_INCLUDE_COM_TELEM */

#endif /* PIOS_INCLUDE_COM */

    /* Configure FlexiPort */
    uint8_t hwsettings_osd_flexiport;
    HwSettingsOSD_FlexiPortGet(&hwsettings_osd_flexiport);

    switch (hwsettings_osd_flexiport) {
    case HWSETTINGS_OSD_FLEXIPORT_DISABLED:
        break;
#if 0
    case HWSETTINGS_OSD_FLEXIPORT_I2C:
#if defined(PIOS_INCLUDE_I2C)
    {
        if (PIOS_I2C_Init(&pios_i2c_flexiport_adapter_id, &pios_i2c_flexiport_adapter_cfg)) {
            PIOS_Assert(0);
        }
    }
#endif /* PIOS_INCLUDE_I2C */
        break;
#endif
    case HWSETTINGS_OSD_FLEXIPORT_TSLRSDEBUG:
#if defined(PIOS_INCLUDE_TSLRSDEBUG)
    {
        uint32_t pios_usart_tslrsdebug_id;
        if (PIOS_USART_Init(&pios_usart_tslrsdebug_id, &pios_usart_tslrsdebug_flexi_cfg)) {
            PIOS_Assert(0);
        }
        uint32_t pios_tslrsdebug_id;
        if (PIOS_TSLRSdebug_Init(&pios_tslrsdebug_id, &pios_tslrsdebug_flexi_cfg, &pios_usart_com_driver, pios_usart_tslrsdebug_id)) {
            PIOS_Assert(0);
        }
    }
#endif /* PIOS_INCLUDE_TSLRSDEBUG */
        break;
    case HWSETTINGS_OSD_FLEXIPORT_PACKETRXOK:
#if defined(PIOS_INCLUDE_PACKETRXOK)
    {
        uint32_t pios_gpio_packetrxok_id;
        if (PIOS_GPIO_Init(&pios_gpio_packetrxok_id, &pios_io_packetrxok_flexi_cfg)) {
            PIOS_Assert(0);
        }
        uint32_t pios_packetrxok_id;
        if (PIOS_PacketRxOk_Init(&pios_packetrxok_id, pios_gpio_packetrxok_id, pios_io_packetrxok_flexi[PIOS_PACKETRXOK_IN].pin.gpio, pios_io_packetrxok_flexi[PIOS_PACKETRXOK_IN].pin.init.GPIO_Pin)) {
            PIOS_Assert(0);
        }
    }
#endif /* PIOS_INCLUDE_PACKETRXOK */
        break;
    }

#if defined(PIOS_INCLUDE_WAVE)
    PIOS_WavPlay_Init(&pios_dac_cfg);
#endif

    // ADC system
#if defined(PIOS_INCLUDE_ADC)
    PIOS_ADC_Init(&pios_adc_cfg);
#endif

#if defined(PIOS_INCLUDE_VIDEO)
    switch (bdinfo->board_rev) {
    case 1:
        PIOS_TIM_InitClock(&tim_8_cfg);
        PIOS_Servo_Init(&pios_servo_cfg);
        break;
    case 2:
        PIOS_Pixel_Init();
        break;
    default:
        PIOS_DEBUG_Assert(0);
    }
    PIOS_Video_Init(&pios_video_cfg);
#endif
}


uint8_t PIOS_Board_Revision(void)
{
    const struct pios_board_info *bdinfo = &pios_board_info_blob;

    return bdinfo->board_rev;
}
