/*
 * board/bananapi/bananapi_m5/bananapi_m5.c
 *
 * (C) Copyright 2019 Hardkernel Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <environment.h>
#include <fdt_support.h>
#include <libfdt.h>
#include <asm/cpu_id.h>
#include <asm/arch/secure_apb.h>
#ifdef CONFIG_SYS_I2C_AML
#include <aml_i2c.h>
#endif
#ifdef CONFIG_AML_VPU
#include <vpu.h>
#endif
#include <vpp.h>
#ifdef CONFIG_AML_HDMITX20
#include <amlogic/hdmi.h>
#endif
#include <asm/arch/eth_setup.h>
#include <phy.h>
#include <linux/mtd/partitions.h>
#include <linux/sizes.h>
#include <asm-generic/gpio.h>
#include <dm.h>
#ifdef CONFIG_AML_SPIFC
#include <amlogic/spifc.h>
#endif
#include <fs.h>

#include <odroid-common.h>

DECLARE_GLOBAL_DATA_PTR;

//new static eth setup
struct eth_board_socket*  eth_board_skt;


int serial_set_pin_port(unsigned long port_base)
{
	//UART in "Always On Module"
	//GPIOAO_0==tx,GPIOAO_1==rx
	//setbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);
	return 0;
}

int dram_init(void)
{
	gd->ram_size = (((readl(AO_SEC_GP_CFG0)) & 0xFFFF0000) << 4);
	return 0;
}

/* secondary_boot_func
 * this function should be write with asm, here, is is only for compiling pass
 * */
void secondary_boot_func(void)
{
}

#ifdef ETHERNET_EXTERNAL_PHY

static int dwmac_meson_cfg_drive_strength(void)
{
	writel(0xaaaaaaa5, P_PAD_DS_REG4A);
	return 0;
}

static void setup_net_chip_ext(void)
{
	eth_aml_reg0_t eth_reg0;
	writel(0x11111111, P_PERIPHS_PIN_MUX_6);
	writel(0x111111, P_PERIPHS_PIN_MUX_7);

	eth_reg0.d32 = 0;
	eth_reg0.b.phy_intf_sel = 1;
	eth_reg0.b.rx_clk_rmii_invert = 0;
	eth_reg0.b.rgmii_tx_clk_src = 0;
	eth_reg0.b.rgmii_tx_clk_phase = 1;
	eth_reg0.b.rgmii_tx_clk_ratio = 4;
	eth_reg0.b.phy_ref_clk_enable = 1;
	eth_reg0.b.clk_rmii_i_invert = 0;
	eth_reg0.b.clk_en = 1;
	eth_reg0.b.adj_enable = 0;
	eth_reg0.b.adj_setup = 0;
	eth_reg0.b.adj_delay = 0;
	eth_reg0.b.adj_skew = 0;
	eth_reg0.b.cali_start = 0;
	eth_reg0.b.cali_rise = 0;
	eth_reg0.b.cali_sel = 0;
	eth_reg0.b.rgmii_rx_reuse = 0;
	eth_reg0.b.eth_urgent = 0;
	setbits_le32(P_PREG_ETH_REG0, eth_reg0.d32);// rmii mode

	setbits_le32(HHI_GCLK_MPEG1, 0x1 << 3);
	/* power on memory */
	clrbits_le32(HHI_MEM_PD_REG0, (1 << 3) | (1<<2));
}
#endif
extern struct eth_board_socket* eth_board_setup(char *name);
extern int designware_initialize(ulong base_addr, u32 interface);

int board_eth_init(bd_t *bis)
{
#ifdef ETHERNET_EXTERNAL_PHY
	dwmac_meson_cfg_drive_strength();
	setup_net_chip_ext();
#endif
	udelay(1000);
	designware_initialize(ETH_BASE, PHY_INTERFACE_MODE_RMII);
	
	return 0;
}

#if CONFIG_AML_SD_EMMC
#include <mmc.h>
#include <asm/arch/sd_emmc.h>
static int  sd_emmc_init(unsigned port)
{
	switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
			//todo add card detect
			/* check card detect */
			clrbits_le32(P_PERIPHS_PIN_MUX_9, 0xF << 24);
			setbits_le32(P_PREG_PAD_GPIO1_EN_N, 1 << 6);
			setbits_le32(P_PAD_PULL_UP_EN_REG1, 1 << 6);
			setbits_le32(P_PAD_PULL_UP_REG1, 1 << 6);
			break;
		case SDIO_PORT_C:
			//enable pull up
			//clrbits_le32(P_PAD_PULL_UP_REG3, 0xff<<0);
			break;
		default:
			break;
	}

	return cpu_sd_emmc_init(port);
}

extern unsigned sd_debug_board_1bit_flag;


static void sd_emmc_pwr_prepare(unsigned port)
{
	cpu_sd_emmc_pwr_prepare(port);
}

static void sd_emmc_pwr_on(unsigned port)
{
	switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
			break;
		case SDIO_PORT_C:
			break;
		default:
			break;
	}
	return;
}
static void sd_emmc_pwr_off(unsigned port)
{
	/// @todo NOT FINISH
	switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
			//            setbits_le32(P_PREG_PAD_GPIO5_O,(1<<31)); //CARD_8
			//            clrbits_le32(P_PREG_PAD_GPIO5_EN_N,(1<<31));
			break;
		case SDIO_PORT_C:
			break;
		default:
			break;
	}
	return;
}

// #define CONFIG_TSD      1
static void board_mmc_register(unsigned port)
{
	struct aml_card_sd_info *aml_priv=cpu_sd_emmc_get(port);
	if (aml_priv == NULL)
		return;

	aml_priv->sd_emmc_init=sd_emmc_init;
	aml_priv->sd_emmc_detect=sd_emmc_detect;
	aml_priv->sd_emmc_pwr_off=sd_emmc_pwr_off;
	aml_priv->sd_emmc_pwr_on=sd_emmc_pwr_on;
	aml_priv->sd_emmc_pwr_prepare=sd_emmc_pwr_prepare;
	aml_priv->desc_buf = malloc(NEWSD_MAX_DESC_MUN*(sizeof(struct sd_emmc_desc_info)));

	if (NULL == aml_priv->desc_buf)
		printf(" desc_buf Dma alloc Fail!\n");
	else
		printf("aml_priv->desc_buf = 0x%p\n",aml_priv->desc_buf);

	sd_emmc_register(aml_priv);
}

static void board_mmc_power_enable(void)
{
	printf("BPI-sd: set sd power on\n");

	/* set gpioH_8 output/external pull high to power on sd vdd_en*/
	writel(readl(PREG_PAD_GPIO3_EN_N) | (1 << 8), PREG_PAD_GPIO3_EN_N);
	writel(readl(PERIPHS_PIN_MUX_C) & (~(0xf)), PERIPHS_PIN_MUX_C);

	/* set gpioE_2 output/high to power on sd vddio */
	writel(readl(AO_GPIO_O) | (1 << 18), AO_GPIO_O);
	writel(readl(AO_GPIO_O_EN_N) | (1 << 18), AO_GPIO_O_EN_N);
	writel(readl(AO_RTI_PINMUX_REG1) & (~(0xf << 24)), AO_RTI_PINMUX_REG1);

	/* set gpioAO_6 output/low to set sd 3v3_1v8_en to 3v3 default */
	writel(readl(AO_GPIO_O) & (~(1 << 6)), AO_GPIO_O);
	writel(readl(AO_GPIO_O_EN_N) & (~(1 << 6)), AO_GPIO_O_EN_N);
	writel(readl(AO_RTI_PINMUX_REG0) & (~(0xf << 24)), AO_RTI_PINMUX_REG0);
}

int board_mmc_init(bd_t	*bis)
{
	board_mmc_power_enable();
	board_mmc_register(SDIO_PORT_C);	// eMMC
	board_mmc_register(SDIO_PORT_B);	// SD card

	return 0;
}
#endif

#ifdef CONFIG_SYS_I2C_AML

#ifdef CONFIG_SYS_I2C_AML_I2C3
static void board_i2c_set_pinmux(void)
{
	/* i2c_3 GPIO_A14/A15 */
	clrbits_le32(PERIPHS_PIN_MUX_E, 0xf << 24 | 0xf << 28);
	setbits_le32(PERIPHS_PIN_MUX_E, MESON_I2C_MASTER_D_GPIOA_14_BIT | MESON_I2C_MASTER_D_GPIOA_15_BIT);
}

struct aml_i2c_platform g_aml_i2c_plat = {
	.wait_count         = 1000000,
	.wait_ack_interval  = 5,
	.wait_read_interval = 5,
	.wait_xfer_interval = 5,
	.master_no          = AML_I2C_MASTER_D,
	.use_pio            = 0,
	.master_i2c_speed   = AML_I2C_SPPED_400K,
	.master_d_pinmux = {
		.scl_reg    = (unsigned long)MESON_I2C_MASTER_D_GPIOA_15_REG,
		.scl_bit    = MESON_I2C_MASTER_D_GPIOA_15_BIT,
		.sda_reg    = (unsigned long)MESON_I2C_MASTER_D_GPIOA_14_REG,
		.sda_bit    = MESON_I2C_MASTER_D_GPIOA_14_BIT,
	}
};
#endif

#ifdef CONFIG_SYS_I2C_AML_I2C2
static void board_i2c_set_pinmux(void)
{	
	/* i2c_2 GPIO_X17/X18  */
	clrbits_le32(PERIPHS_PIN_MUX_5, 0xf << 4 | 0xf << 8);
	setbits_le32(PERIPHS_PIN_MUX_5, MESON_I2C_MASTER_C_GPIOX_17_BIT | MESON_I2C_MASTER_C_GPIOX_18_BIT);
}

struct aml_i2c_platform g_aml_i2c_plat = {
	.wait_count         = 1000000,
	.wait_ack_interval  = 5,
	.wait_read_interval = 5,
	.wait_xfer_interval = 5,
	.master_no          = AML_I2C_MASTER_C,
	.use_pio            = 0,
	.master_i2c_speed   = AML_I2C_SPPED_400K,
	.master_c_pinmux = {
		.scl_reg    = (unsigned long)MESON_I2C_MASTER_C_GPIOX_18_REG,
		.scl_bit    = MESON_I2C_MASTER_C_GPIOX_18_BIT,
		.sda_reg    = (unsigned long)MESON_I2C_MASTER_C_GPIOX_17_REG,
		.sda_bit    = MESON_I2C_MASTER_C_GPIOX_17_BIT,
	}
};
#endif

#ifdef CONFIG_SYS_I2C_AML_I2C1
static void board_i2c_set_pinmux(void)
{
	/* i2c_2 GPIO_X10/X11  */
	clrbits_le32(PERIPHS_PIN_MUX_4, 0xf << 8 | 0xf << 12);
	setbits_le32(PERIPHS_PIN_MUX_4, MESON_I2C_MASTER_B_GPIOX_10_BIT | MESON_I2C_MASTER_B_GPIOX_11_BIT);
}

struct aml_i2c_platform g_aml_i2c_plat = {
	.wait_count         = 1000000,
	.wait_ack_interval  = 5,
	.wait_read_interval = 5,
	.wait_xfer_interval = 5,
	.master_no          = AML_I2C_MASTER_B,
	.use_pio            = 0,
	.master_i2c_speed   = AML_I2C_SPPED_400K,
	.master_b_pinmux = {
		.scl_reg    = (unsigned long)MESON_I2C_MASTER_B_GPIOX_11_REG,
		.scl_bit    = MESON_I2C_MASTER_B_GPIOX_11_BIT,
		.sda_reg    = (unsigned long)MESON_I2C_MASTER_B_GPIOX_10_REG,
		.sda_bit    = MESON_I2C_MASTER_B_GPIOX_10_BIT,
	}
};
#endif


static void board_i2c_init(void)
{
#if defined(CONFIG_SYS_I2C_AML_I2C2) || defined(CONFIG_SYS_I2C_AML_I2C3) || defined(CONFIG_SYS_I2C_AML_I2C1)
	board_i2c_set_pinmux();
	i2c_plat_init();
#endif

	aml_i2c_init();
	udelay(10);
}
#endif

#if defined(CONFIG_BOARD_EARLY_INIT_F)
int board_early_init_f(void){
	/*add board early init function here*/
	return 0;
}
#endif

#ifdef CONFIG_USB_XHCI_AMLOGIC_V2
#include <asm/arch/usb-v2.h>
#include <asm/arch/gpio.h>
#define CONFIG_GXL_USB_U2_PORT_NUM	2

#ifdef CONFIG_USB_XHCI_AMLOGIC_USB3_V2
#define CONFIG_GXL_USB_U3_PORT_NUM	1
#else
#define CONFIG_GXL_USB_U3_PORT_NUM	0
#endif

static void gpio_set_vbus_power(char is_power_on)
{
	int ret;

	/* USB Host power enable/disable */
	usbhost_set_power(is_power_on);

	/* usb otg power enable */
	ret = gpio_request(CONFIG_USB_GPIO_PWR,
			CONFIG_USB_GPIO_PWR_NAME);
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n",
				CONFIG_USB_GPIO_PWR);
		return;
	}

	gpio_direction_output(CONFIG_USB_GPIO_PWR, !!is_power_on);
}

struct amlogic_usb_config g_usb_config_GXL_skt={
	CONFIG_GXL_XHCI_BASE,
	USB_ID_MODE_HARDWARE,
	gpio_set_vbus_power,//gpio_set_vbus_power, //set_vbus_power
	CONFIG_GXL_USB_PHY2_BASE,
	CONFIG_GXL_USB_PHY3_BASE,
	CONFIG_GXL_USB_U2_PORT_NUM,
	CONFIG_GXL_USB_U3_PORT_NUM,
	.usb_phy2_pll_base_addr = {
		CONFIG_USB_PHY_20,
		CONFIG_USB_PHY_21,
	}
};

#endif /*CONFIG_USB_XHCI_AMLOGIC*/

#ifdef CONFIG_AML_HDMITX20
static void hdmi_tx_set_hdmi_5v(void)
{
}
#endif

#ifdef CONFIG_AML_SPIFC
/*
 * BOOT_3: NOR_HOLDn:reg0[15:12]=3
 * BOOT_4: NOR_D:reg0[19:16]=3
 * BOOT_5: NOR_Q:reg0[23:20]=3
 * BOOT_6: NOR_C:reg0[27:24]=3
 * BOOT_7: NOR_WPn:reg0[31:28]=3
 * BOOT_14: NOR_CS:reg1[27:24]=3
 */
#define SPIFC_NUM_CS 1
static int spifc_cs_gpios[SPIFC_NUM_CS] = {54};

static int spifc_pinctrl_enable(void *pinctrl, bool enable)
{
	unsigned int val;

	val = readl(P_PERIPHS_PIN_MUX_0);
	val &= ~(0xfffff << 12);
	if (enable)
		val |= 0x33333 << 12;
	writel(val, P_PERIPHS_PIN_MUX_0);

	val = readl(P_PERIPHS_PIN_MUX_1);
	val &= ~(0xf << 24);
	writel(val, P_PERIPHS_PIN_MUX_1);
	return 0;
}

static const struct spifc_platdata spifc_platdata = {
	.reg = 0xffd14000,
	.mem_map = 0xf6000000,
	.pinctrl_enable = spifc_pinctrl_enable,
	.num_chipselect = SPIFC_NUM_CS,
	.cs_gpios = spifc_cs_gpios,
};

U_BOOT_DEVICE(spifc) = {
	.name = "spifc",
	.platdata = &spifc_platdata,
};
#endif /* CONFIG_AML_SPIFC */

int board_init(void)
{
	board_led_alive(1);

#ifdef CONFIG_SYS_I2C_AML
	board_i2c_init();
#endif

#ifdef CONFIG_USB_XHCI_AMLOGIC_V2
	board_usb_pll_disable(&g_usb_config_GXL_skt);
	board_usb_init(&g_usb_config_GXL_skt,BOARD_USB_MODE_HOST);
#endif /*CONFIG_USB_XHCI_AMLOGIC*/

	return 0;
}

#if !defined(CONFIG_FASTBOOT_FLASH_MMC_DEV)
#define CONFIG_FASTBOOT_FLASH_MMC_DEV		0
#endif

extern void cvbs_init(void);

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#if defined(CONFIG_FASTBOOT_FLASH_MMC_DEV)
	/* select the default mmc device */
	int mmc_devnum = CONFIG_FASTBOOT_FLASH_MMC_DEV;

	if (get_boot_device() == BOOT_DEVICE_EMMC)
		mmc_devnum = 0;
	else if (get_boot_device() == BOOT_DEVICE_SD)
		mmc_devnum = 1;

	/* select the default mmc device */
	mmc_select_hwpart(mmc_devnum, 0);
#endif
#ifdef CONFIG_AML_VPU
	vpu_probe();
#endif
	vpp_init();
#ifdef CONFIG_AML_HDMITX20
	hdmi_tx_set_hdmi_5v();
	hdmi_tx_init();
#endif

#ifdef CONFIG_AML_CVBS
	run_command("cvbs init; cvbs output 480cvbs", 0);
	board_cvbs_probe();
#endif

	if (board_is_bananapi_m5()) {
		printf("BPI: board is Bananapi M5\n");
		setenv("variant", "bananapi_m5");
		setenv("board", "bpi-m5");
	}
	else if (board_is_bananapi_m2_pro()) {
		printf("BPI: board is Bananapi M2 Pro\n");
		setenv("variant", "bananapi_m2_pro");
		setenv("board", "bpi-m2pro");
	}

	board_set_dtbfile("meson64_%s.dtb");

	if (get_boot_device() == BOOT_DEVICE_SPI) {
		setenv("bootdelay", "0");
		setenv("bootcmd", "run boot_spi");
		run_command("sf probe", 0);

		if (file_exists("mmc", "1:1", "petitboot.cfg", FS_TYPE_ANY)) {
			run_command("load mmc 1:1 $loadaddr petitboot.cfg", 0);
			run_command("ini u-boot", 0);
			run_command("ini petitboot", 0);
			if (!strcmp("true", getenv("overwrite")))
				saveenv();
		}
	}

	/* boot logo display - 1080p60hz */
	run_command("showlogo", 0);
	usbhost_early_poweron();

	return 0;
}
#endif

/* SECTION_SHIFT is 29 that means 512MB size */
#define SECTION_SHIFT		29
phys_size_t get_effective_memsize(void)
{
	phys_size_t size_aligned;

	size_aligned = (((readl(AO_SEC_GP_CFG0)) & 0xFFFF0000) << 4);
	size_aligned = ((size_aligned >> SECTION_SHIFT) << SECTION_SHIFT);

#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	size_aligned = size_aligned - CONFIG_SYS_MEM_TOP_HIDE;
#endif

	return size_aligned;
}
