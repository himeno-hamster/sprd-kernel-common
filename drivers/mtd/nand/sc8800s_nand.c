/* linux/drivers/mtd/nand/sprd8800.c
 *
 * Copyright (c) 2010 Spreadtrun.
 *
 * Spreadtrun 8800 NAND driver

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <mach/regs_ahb.h>
#include <mach/regs_nfc.h>
#include <mach/regs_cpc.h>
#include <mach/regs_global.h>


/* NandFlash command */
#define NF_READ_STATUS  	0x70

#define NF_PARA_20M        	0x7ac05      //trwl = 0  trwh = 0
#define NF_PARA_40M        	0x7ac15      //trwl = 1  trwh = 0
#define NF_PARA_53M        	0x7ad26      //trwl = 2  trwh = 1
#define NF_PARA_80M        	0x7ad37      //trwl = 3  trwh = 1
#define NF_PARA_DEFAULT    	0x7ad77      //trwl = 7  trwh = 1
#define NF_TIMEOUT_VAL 		0x1000000

#define PAGE_SIZE_S         512
#define SPARE_SIZE_S        16
#define PAGE_SIZE_L         2048
#define SPARE_SIZE_L        64

#define BLOCK_TOTAL         1024
#define PAGEPERBLOCK	    64

#define REG_CPC_NFWPN				(*((volatile unsigned int *)(CPC_NFWPN_REG)))
#define REG_CPC_NFRB				(*((volatile unsigned int *)(CPC_NFRB_REG)))
#define REG_CPC_NFCLE                           (*((volatile unsigned int *)(CPC_NFCLE_REG)))
#define REG_CPC_NFALE				(*((volatile unsigned int *)(CPC_NFALE_REG)))
#define REG_CPC_NFCEN                           (*((volatile unsigned int *)(CPC_NFCEN_REG)))
#define REG_CPC_NFWEN                           (*((volatile unsigned int *)(CPC_NFWEN_REG)))
#define REG_CPC_NFREN                           (*((volatile unsigned int *)(CPC_NFREN_REG)))
#define REG_CPC_NFD0                            (*((volatile unsigned int *)(CPC_NFD0_REG)))
#define REG_CPC_NFD1                            (*((volatile unsigned int *)(CPC_NFD1_REG)))
#define REG_CPC_NFD2                            (*((volatile unsigned int *)(CPC_NFD2_REG)))
#define REG_CPC_NFD3                            (*((volatile unsigned int *)(CPC_NFD3_REG)))
#define REG_CPC_NFD4                            (*((volatile unsigned int *)(CPC_NFD4_REG)))
#define REG_CPC_NFD5                            (*((volatile unsigned int *)(CPC_NFD5_REG)))
#define REG_CPC_NFD6                            (*((volatile unsigned int *)(CPC_NFD6_REG)))
#define REG_CPC_NFD7                            (*((volatile unsigned int *)(CPC_NFD7_REG)))
#define REG_CPC_NFD8                            (*((volatile unsigned int *)(CPC_NFD8_REG)))

#define REG_AHB_CTL0		       		(*((volatile unsigned int *)(AHB_CTL0)))

#define REG_GR_NFC_MEM_DLY                      (*((volatile unsigned int *)(GR_NFC_MEM_DLY)))

#define set_gpio_as_nand()                              \
do {                                                    \
        REG_CPC_NFWPN = BIT_0 | BIT_4 | BIT_5;          \
        REG_CPC_NFWPN &= ~(BIT_6 | BIT_7);              \
        REG_CPC_NFRB = BIT_0 | BIT_3 | BIT_4 | BIT_5;   \
        REG_CPC_NFRB &= ~(BIT_6 | BIT_7);               \
	REG_CPC_NFCLE |= BIT_4 | BIT_5;			\
	REG_CPC_NFCLE &= ~(BIT_6 | BIT_7);		\
	REG_CPC_NFALE |= BIT_4 | BIT_5;                 \
        REG_CPC_NFALE &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFCEN |= BIT_4 | BIT_5;                 \
        REG_CPC_NFCEN &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFWEN |= BIT_4 | BIT_5;                 \
        REG_CPC_NFWEN &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFREN |= BIT_4 | BIT_5;                 \
        REG_CPC_NFREN &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFD0 |= BIT_4 | BIT_5;                 \
        REG_CPC_NFD0 &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFD1 |= BIT_4 | BIT_5;                 \
        REG_CPC_NFD1 &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFD2 |= BIT_4 | BIT_5;                 \
        REG_CPC_NFD2 &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFD3 |= BIT_4 | BIT_5;                 \
        REG_CPC_NFD3 &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFD4 |= BIT_4 | BIT_5;                 \
        REG_CPC_NFD4 &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFD5 |= BIT_4 | BIT_5;                 \
        REG_CPC_NFD5 &= ~(BIT_6 | BIT_7);              \
	REG_CPC_NFD6 |= BIT_4 | BIT_5;                 \
        REG_CPC_NFD6 &= ~(BIT_6 | BIT_7);              \
        REG_CPC_NFD7 |= BIT_4 | BIT_5;                 \
        REG_CPC_NFD7 &= ~(BIT_6 | BIT_7);              \
} while (0)

struct sprd_platform_nand {
	/* timing information for nand flash controller */
	int	acs;
	int 	ach;
	int	rwl;
	int	rwh;
	int	rr;
	int	acr;
	int	ceh;
};

struct sprd_nand_address {
	int column;
	int row;
	int colflag;
	int rowflag;
};

struct sprd_nand_info {
	struct sprd_platform_nand	*platform;
	struct clk	*clk;
};

typedef enum {
	NO_OP,
	WRITE_OP,
	READ_OP,
} sprd_nand_wr_mode_t;

typedef enum {
	NO_AREA,
	DATA_AREA,
	OOB_AREA,
	DATA_OOB_AREA,
} sprd_nand_area_mode_t;

static struct mtd_info *sprd_mtd = NULL;
static unsigned long g_cmdsetting = 0;
static sprd_nand_wr_mode_t sprd_wr_mode = NO_OP;
static sprd_nand_area_mode_t sprd_area_mode = NO_AREA;
static unsigned long nand_flash_id = 0;
static struct sprd_nand_address sprd_colrow_addr = {0, 0, 0, 0};
static unsigned char io_wr_port[NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE];
static nand_ecc_modes_t sprd_ecc_mode = NAND_ECC_NONE;

static struct sprd_platform_nand *to_nand_plat(struct platform_device *dev)
{
	return dev->dev.platform_data;
}

//static unsigned long g_CmdSetting;

#ifdef CONFIG_MTD_PARTITIONS
const char *part_probes[] = { "cmdlinepart", NULL };
#endif

static int nfc_wait_command_finish(void)
{
	unsigned long nfc_cmd = REG_NFC_CMD;
	unsigned long counter = 0;
	
	while ((nfc_cmd & (0x1 << 31)) && (counter < NF_TIMEOUT_VAL)) {
		nfc_cmd = REG_NFC_CMD;
		counter++;
	}
	
	if (NF_TIMEOUT_VAL == counter) {
		return 2;
	}
	
	return 0;
}

static void set_nfc_param(unsigned long ahb_clk)
{
	nfc_wait_command_finish();
	
	switch (ahb_clk) {
	case 20:
        	REG_NFC_PARA = NF_PARA_20M;
        break;
        case 40:
        	REG_NFC_PARA = NF_PARA_40M;
        break;
        case 53:
        	REG_NFC_PARA = NF_PARA_53M;
        break;
        case 80:
        	REG_NFC_PARA = NF_PARA_80M;
        break;
        default:
             	REG_NFC_PARA = NF_PARA_DEFAULT;    
    	}	
}

static void nand_copy(unsigned char *src, unsigned char *dst, unsigned long len)
{
	unsigned long i;
	unsigned long *pDst_32, *pSrc_32;
	unsigned short *pDst_16, *pSrc_16;
	unsigned long flag = 0;
	
	//flag = (unsigned long *)dst;
	flag = (unsigned long)dst;
	flag = flag & 0x3;

	switch (flag) {
		case 0://word alignment
        		pDst_32 = (unsigned long *)dst;
                	pSrc_32 = (unsigned long *)src;
                	for (i = 0; i < (len / 4); i++) {
				*pDst_32 = *pSrc_32;
                    		pDst_32++;
                    		pSrc_32++;
			}
        	break;
        	case 2://half word alignment
                	pDst_16 = (unsigned short *)dst;
                	pSrc_16 = (unsigned short *)src;
                	for (i = 0; i < (len / 2); i++) {
                    		*pDst_16 = *pSrc_16;
                    		pDst_16++;
                    		pSrc_16++;
                	}
            	break;
        	default://byte alignment
                	for (i = 0; i < len; i++) {
                    		*dst = *src;
                    		dst++;
                    		src++;
                	}
            	break;
    	}//switch	
}

static int sprd_nand_inithw(struct sprd_nand_info *info, struct platform_device *pdev)
{
#if 0
	struct sprd_platform_nand *plat = to_nand_plat(pdev);
	unsigned long para = (plat->acs << 0) | 
				(plat->ach << 2) |
				(plat->rwl << 4) |
				(plat->rwh << 8) |
				(plat->rr << 10) |
				(plat->acr << 13) |
				(plat->ceh << 16);

 	writel(para, NFCPARAMADDR);
#endif
	
	return 0;
}

static void sprd_nand_hwcontrol(struct mtd_info *mtd, int cmd,
				   unsigned int ctrl)
{
        unsigned long addr_cycle = 0;
        unsigned long advance = 1;
        unsigned long buswidth = 0;
        unsigned long pagetype = 1;
	unsigned long phyblk, pageinblk, pageperblk;
	
	unsigned char i;
	
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	if (cmd == NAND_CMD_NONE) {
		return;
	}
	if (ctrl & NAND_CLE) {
		switch (cmd) {
			case NAND_CMD_RESET:
				REG_NFC_CMD = cmd | (0x1 << 31);
				nfc_wait_command_finish();
				mdelay(2);
			break;
			case NAND_CMD_STATUS:
				REG_NFC_CMD = cmd | (0x1 << 31);
				nfc_wait_command_finish();
				
				memset((unsigned char *)(this->IO_ADDR_R), 0xff, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);
				nand_copy((unsigned char *)NFC_IDSTATUS, this->IO_ADDR_R, 4);
				/* transfer to litter end */
				i = io_wr_port[3]; io_wr_port[3] = io_wr_port[0]; io_wr_port[0] = i;
				i = io_wr_port[2]; io_wr_port[2] = io_wr_port[1]; io_wr_port[1] = i;

        			/*for (i = 0; i < 4; i++)
                			printk("io_wr_port[%d] = 0x%02x\n", i, io_wr_port[i]);*/
			break;
			case NAND_CMD_READID:
        			REG_NFC_CMD = cmd | (0x1 << 31);
        			nfc_wait_command_finish();
        			nand_flash_id = REG_NFC_IDSTATUS;
			break;
			case NAND_CMD_ERASE1:
				sprd_colrow_addr.column = 0;
				sprd_colrow_addr.row = 0;
				sprd_colrow_addr.colflag = 0;
				sprd_colrow_addr.rowflag = 0;
			break;
			case NAND_CMD_ERASE2:
				if ((0 == sprd_colrow_addr.colflag) && (0 == sprd_colrow_addr.rowflag)) {
					printk("erase address error!\n");
					return;
				} else {
					if (1 == sprd_colrow_addr.colflag) {
						sprd_colrow_addr.row = sprd_colrow_addr.column;
						sprd_colrow_addr.column = 0;
						sprd_colrow_addr.rowflag = 1;
						sprd_colrow_addr.colflag = 0;	
					}	
				}
				
				if ((0 == sprd_colrow_addr.colflag) && (1 == sprd_colrow_addr.rowflag)) {
					g_cmdsetting = (addr_cycle << 24) | (advance << 23) | (buswidth << 19) \
							| (pagetype << 18) | (0 << 16) | (0x1 << 31);

        				REG_NFC_STR0 = sprd_colrow_addr.row * mtd->writesize * 2;
        				REG_NFC_CMD = g_cmdsetting | NAND_CMD_ERASE1;
        				nfc_wait_command_finish();
				}
			break;
			case NAND_CMD_READ0:
				sprd_colrow_addr.column = 0;
				sprd_colrow_addr.row = 0;
				sprd_colrow_addr.colflag = 0;
				sprd_colrow_addr.rowflag = 0;
				sprd_wr_mode = READ_OP;
				sprd_area_mode = NO_AREA;
				//memset((unsigned char *)(this->IO_ADDR_R), 0xff, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);
				memset(io_wr_port, 0xff, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);
			break;
			case NAND_CMD_READSTART:
				if (sprd_colrow_addr.column == mtd->writesize) {
        				g_cmdsetting = (addr_cycle << 24) | (advance << 23) | (buswidth << 19) | \
							(pagetype << 18) | (0 << 16) | (0x1 << 31);
        				REG_NFC_CMD = g_cmdsetting | NAND_CMD_READ0;
        				nfc_wait_command_finish();
				
					//nand_copy((unsigned char *)NFC_SBUF, this->IO_ADDR_R, mtd->oobsize);
					nand_copy((unsigned char *)NFC_SBUF, io_wr_port, mtd->oobsize);

        				/*for (i = 0; i < mtd->oobsize; i++)
                				printk(" Rport[%d]=%d ", i, io_wr_port[i]);*/
				} else if (sprd_colrow_addr.column == 0) {
						if (sprd_area_mode == DATA_AREA)
							sprd_area_mode = DATA_OOB_AREA;

						if (sprd_area_mode == DATA_OOB_AREA) {
                                                	/* read data and spare area, modify address */
                                                	/*REG_NFC_END0 = phyblk * pageperblk * mtd->writesize * 2 +
 									pageinblk * mtd->writesize * 2 + 
									mtd->writesize + mtd->writesize - 1; */
                                                	REG_NFC_END0 = 0xffffffff;
                                                	g_cmdsetting = (addr_cycle << 24) | (advance << 23) | \
									(1 << 21) | (buswidth << 19) | (pagetype << 18) | \
									(0 << 16) | (0x1 << 31);

                                                	REG_NFC_CMD = g_cmdsetting | NAND_CMD_READ0;
                                                	nfc_wait_command_finish();
							
							nand_copy((unsigned char *)NFC_MBUF, io_wr_port, mtd->writesize);
							/*nand_copy((unsigned char *)NFC_SBUF, io_wr_port + mtd->writesize, 
								mtd->oobsize);*/

                                        	} else if (sprd_colrow_addr.column == DATA_AREA) {
        						g_cmdsetting = (addr_cycle << 24) | (advance << 23) | \
									(buswidth << 19) | (pagetype << 18) | \
									(0 << 16) | (0x1 << 31);
        						REG_NFC_CMD = g_cmdsetting | NAND_CMD_READ0;
        						nfc_wait_command_finish();
				
							nand_copy((unsigned char *)NFC_MBUF, io_wr_port, mtd->writesize);

        						/*for (i = 0; i < mtd->writesize; i++)
                						printk(" Rport[%d]=%d ", i, io_wr_port[i]);*/
					}
				} else
					printk("Operation !!! area.  %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
				sprd_wr_mode = NO_OP;
				sprd_area_mode = NO_AREA;
			break;
			case NAND_CMD_SEQIN:
				sprd_colrow_addr.column = 0;
				sprd_colrow_addr.row = 0;
				sprd_colrow_addr.colflag = 0;
				sprd_colrow_addr.rowflag = 0;
				sprd_wr_mode = WRITE_OP;
				sprd_area_mode = NO_AREA;
				//memset((unsigned char *)(this->IO_ADDR_W), 0xff, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);
				memset(io_wr_port, 0xff, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);
			break;
			case NAND_CMD_PAGEPROG:
				if (sprd_colrow_addr.column == mtd->writesize) {
					printk("%s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
        				/*for (i = 0; i < mtd->oobsize; i++)
                				printk(" Wport[%d]=%d ", i, io_wr_port[i]);*/

        				g_cmdsetting = (addr_cycle << 24) | (advance << 23) | (buswidth << 19) | \
							(pagetype << 18) | (0 << 16) | (0x1 << 31);
        			
					//nand_copy(this->IO_ADDR_W, (unsigned char *)NFC_SBUF, mtd->oobsize);
					nand_copy(io_wr_port, (unsigned char *)NFC_SBUF, mtd->oobsize);
        				REG_NFC_CMD = g_cmdsetting | NAND_CMD_SEQIN;
        				nfc_wait_command_finish();
				} else if (sprd_colrow_addr.column == 0) {
					if (sprd_area_mode == DATA_OOB_AREA) {
						/* write data and spare area, modify address */
        					/*REG_NFC_END0 = phyblk * pageperblk * mtd->writesize * 2 + 
								pageinblk * mtd->writesize * 2 + 
								mtd->writesize + mtd->writesize - 1;*/
						REG_NFC_END0 = 0xffffffff;
        					g_cmdsetting = (addr_cycle << 24) | (advance << 23) | (buswidth << 19) | \
								(pagetype << 18) | (0 << 16) | (0x1 << 31);
        			
        					REG_NFC_CMD = g_cmdsetting | NAND_CMD_SEQIN;
        					nfc_wait_command_finish();
					
					} else if (sprd_colrow_addr.column == DATA_AREA) {
        					g_cmdsetting = (addr_cycle << 24) | (advance << 23) | (buswidth << 19) | \
								(pagetype << 18) | (0 << 16) | (0x1 << 31);
        			
						//nand_copy(this->IO_ADDR_W, (unsigned char *)NFC_MBUF, mtd->writesize);
						nand_copy(io_wr_port, (unsigned char *)NFC_MBUF, mtd->writesize);
        					REG_NFC_CMD = g_cmdsetting | NAND_CMD_SEQIN;
        					nfc_wait_command_finish();
					}
				} else
					printk("Operation !!! area.  %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
				sprd_wr_mode = NO_OP;
				sprd_area_mode = NO_AREA;
			break;
			default:
			break;	
		}
	} else {
		if (0 == sprd_colrow_addr.colflag) {
			sprd_colrow_addr.colflag = 1;
			sprd_colrow_addr.column = cmd;
			//printk("%s  %s  %d   column = %d\n", __FILE__, __FUNCTION__, __LINE__, cmd);
			return;
		}
		
		if (0 == sprd_colrow_addr.rowflag) {
			sprd_colrow_addr.rowflag = 1;
			sprd_colrow_addr.row = cmd;
			//printk("%s  %s  %d  row = %d\n", __FILE__, __FUNCTION__, __LINE__, cmd);
		}
		
		if ((1 == sprd_colrow_addr.colflag) && (1 == sprd_colrow_addr.rowflag)) {
			if (sprd_colrow_addr.column == mtd->writesize) {
				pageperblk = mtd->erasesize / mtd->writesize;
				phyblk = sprd_colrow_addr.row / pageperblk;
        			pageinblk = sprd_colrow_addr.row % pageperblk;

        			REG_NFC_STR0 = phyblk * pageperblk * mtd->writesize * 2 + 
							pageinblk * mtd->writesize * 2 + 
							sprd_colrow_addr.column;
        			REG_NFC_END0 = phyblk * pageperblk * mtd->writesize * 2 + 
							pageinblk * mtd->writesize * 2 + 
							sprd_colrow_addr.column + mtd->oobsize -1;
				sprd_area_mode = OOB_AREA;	
				/*printk("Operation OOB area.  %s  %s  %d   row=0x%08x  column=0x%08x\n", 
					__FILE__, __FUNCTION__, __LINE__, sprd_colrow_addr.row, sprd_colrow_addr.column);*/
			} else if (sprd_colrow_addr.column == 0) {
				/*REG_NFC_STR0 = sprd_colrow_addr.row * mtd->writesize + sprd_colrow_addr.column;
				REG_NFC_END0 = sprd_colrow_addr.row * mtd->writesize + sprd_colrow_addr.column 
						+ mtd->writesize - 1;*/
				
				pageperblk = mtd->erasesize / mtd->writesize;
				phyblk = sprd_colrow_addr.row / pageperblk;
        			pageinblk = sprd_colrow_addr.row % pageperblk;

        			REG_NFC_STR0 = phyblk * pageperblk * mtd->writesize * 2 + 
							pageinblk * mtd->writesize * 2 + 
							sprd_colrow_addr.column;
        			REG_NFC_END0 = phyblk * pageperblk * mtd->writesize * 2 + 
							pageinblk * mtd->writesize * 2 + 
							sprd_colrow_addr.column + mtd->writesize - 1;
				sprd_area_mode = DATA_AREA;	
				/*printk("Operation DATA area.  %s  %s  %d   row=0x%08x  column=0x%08x\n", 
					__FILE__, __FUNCTION__, __LINE__, sprd_colrow_addr.row, sprd_colrow_addr.column);*/
			} else
				printk("Operation ??? area.  %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
		}
	}		
}

static unsigned long sprd_nand_readid(struct mtd_info *mtd)
{
	return(nand_flash_id);
}

static int sprd_nand_devready(struct mtd_info *mtd)
{
	unsigned long status = 0;
        unsigned long cmd = NF_READ_STATUS | (0x1 << 31);

        REG_NFC_CMD = cmd;
        nfc_wait_command_finish();

        status = REG_NFC_IDSTATUS;
		
   	if ((status & 0x1) != 0) 	
     		return -1; /* fail */
   	else if ((status & 0x40) == 0)
     		return 0; /* busy */
   	else 							
     		return 1; /* ready */
}

static void sprd_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct nand_chip *this = mtd->priv;
	struct sprd_nand_info *info = this->priv;

#if 0	
	if (chip != -1)
		clk_enable(info->clk);
	else
		clk_disable(info->clk);
#endif
}

/* ecc function */
static void sprd_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	sprd_ecc_mode = mode;
}

static unsigned long sprd_nand_wr_oob(struct mtd_info *mtd)
{
        /* copy io_wr_port into SBUF */
        nand_copy(io_wr_port, (unsigned char *)NFC_SBUF, mtd->oobsize);

	/* write oob area */
	if (sprd_area_mode == NO_AREA)
		sprd_area_mode = OOB_AREA;
	else if (sprd_area_mode == DATA_AREA)
		sprd_area_mode = DATA_OOB_AREA;

	return 0;
}

static int sprd_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
	unsigned char ecc_val_in[16];
        unsigned long *pecc_val;			

	if (sprd_ecc_mode == NAND_ECC_WRITE) {
		pecc_val = (unsigned long *)ecc_val_in;

		REG_NFC_ECCEN = 0x1;
		/* copy io_wr_port into MBUF */
		nand_copy(io_wr_port, (unsigned char *)NFC_MBUF, mtd->writesize);
		/* large page */
		pecc_val[0] = REG_NFC_PAGEECC0;
               	pecc_val[1] = REG_NFC_PAGEECC1;
               	pecc_val[2] = REG_NFC_PAGEECC2;
               	pecc_val[3] = REG_NFC_PAGEECC3;
		
		/*printk("\nECC0 = 0x%08x  ", REG_NFC_PAGEECC0);		
		printk("ECC1 = 0x%08x   ", REG_NFC_PAGEECC1);		
		printk("ECC2 = 0x%08x   ", REG_NFC_PAGEECC2);		
		printk("ECC3 = 0x%08x\n", REG_NFC_PAGEECC3);*/		

#if 0
		/* little endian */
		ecc_code[0] = ecc_val_in[0];
		ecc_code[1] = ecc_val_in[1];
		ecc_code[2] = ecc_val_in[2];

		ecc_code[3] = ecc_val_in[4];
		ecc_code[4] = ecc_val_in[5];
		ecc_code[5] = ecc_val_in[6];

		ecc_code[6] = ecc_val_in[8];
		ecc_code[7] = ecc_val_in[9];
		ecc_code[8] = ecc_val_in[10];

		ecc_code[9] = ecc_val_in[12];
		ecc_code[10] = ecc_val_in[13];
		ecc_code[11] = ecc_val_in[14];
#else
		/* big endian */
		ecc_code[0] = ecc_val_in[3];
		ecc_code[1] = ecc_val_in[2];
		ecc_code[2] = ecc_val_in[1];

		ecc_code[3] = ecc_val_in[7];
		ecc_code[4] = ecc_val_in[6];
		ecc_code[5] = ecc_val_in[5];

		ecc_code[6] = ecc_val_in[11];
		ecc_code[7] = ecc_val_in[10];
		ecc_code[8] = ecc_val_in[9];

		ecc_code[9] = ecc_val_in[15];
		ecc_code[10] = ecc_val_in[14];
		ecc_code[11] = ecc_val_in[13];
#endif
		REG_NFC_ECCEN = 0;
		memset(io_wr_port, 0xff, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);	
	} else if (sprd_ecc_mode == NAND_ECC_READ) {
 		pecc_val = (unsigned long *)ecc_val_in;
                /* large page */
                pecc_val[0] = REG_NFC_PAGEECC0;
                pecc_val[1] = REG_NFC_PAGEECC1;
                pecc_val[2] = REG_NFC_PAGEECC2;
                pecc_val[3] = REG_NFC_PAGEECC3;

                /*printk("\nECC0 = 0x%08x  ", REG_NFC_PAGEECC0);
                printk("ECC1 = 0x%08x   ", REG_NFC_PAGEECC1);
                printk("ECC2 = 0x%08x   ", REG_NFC_PAGEECC2);
                printk("ECC3 = 0x%08x\n", REG_NFC_PAGEECC3);*/

#if 0
                /* little endian */
                ecc_code[0] = ecc_val_in[0];
                ecc_code[1] = ecc_val_in[1];
                ecc_code[2] = ecc_val_in[2];

                ecc_code[3] = ecc_val_in[4];
                ecc_code[4] = ecc_val_in[5];
                ecc_code[5] = ecc_val_in[6];

                ecc_code[6] = ecc_val_in[8];
                ecc_code[7] = ecc_val_in[9];
                ecc_code[8] = ecc_val_in[10];

                ecc_code[9] = ecc_val_in[12];
                ecc_code[10] = ecc_val_in[13];
                ecc_code[11] = ecc_val_in[14];
#else
                /* big endian */
                ecc_code[0] = ecc_val_in[3];
                ecc_code[1] = ecc_val_in[2];
                ecc_code[2] = ecc_val_in[1];

                ecc_code[3] = ecc_val_in[7];
                ecc_code[4] = ecc_val_in[6];
                ecc_code[5] = ecc_val_in[5];

                ecc_code[6] = ecc_val_in[11];
                ecc_code[7] = ecc_val_in[10];
                ecc_code[8] = ecc_val_in[9];

                ecc_code[9] = ecc_val_in[15];
                ecc_code[10] = ecc_val_in[14];
                ecc_code[11] = ecc_val_in[13];
#endif
                memset(io_wr_port, 0xff, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);
		nand_copy((unsigned char *)NFC_SBUF, io_wr_port, mtd->oobsize);
	}	

	/*for (j = 0; j < 12; j++)
		printk(" ecc_code[%d]=%02x ", j, ecc_code[j]);*/
	sprd_ecc_mode = NAND_ECC_NONE;

	return 0;
}

static int correct(u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
	unsigned int diff0, diff1, diff2;
	unsigned int bit, byte;

	diff0 = read_ecc[0] ^ calc_ecc[0];
	diff1 = read_ecc[1] ^ calc_ecc[1];
	diff2 = read_ecc[2] ^ calc_ecc[2];
	
	//printk("diff0=0x%08x  diff1=0x%08x  diff2=0x%08x\n", diff0, diff1, diff2);

	if (diff0 == 0 && diff1 == 0 && diff2 == 0) {
		return 0;
	}
	if (((diff0 ^ (diff0 >> 1)) & 0x55) == 0x55 &&
	    ((diff1 ^ (diff1 >> 1)) & 0x55) == 0x55 &&
	    ((diff2 ^ (diff2 >> 1)) & 0x55) == 0x55) {
		/* calculate the bit position of the error */
		bit  = (diff2 >> 2) & 1;
		bit |= (diff2 >> 3) & 2;
		bit |= (diff2 >> 4) & 4;
		/* calculate the byte position of the error */
		byte  = (diff1 << 1) & 0x80;
		byte |= (diff1 << 2) & 0x40;
		byte |= (diff1 << 3) & 0x20;
		byte |= (diff1 << 4) & 0x10;
		byte |= (diff0 >> 3) & 0x08;
		byte |= (diff0 >> 2) & 0x04;
		byte |= (diff0 >> 1) & 0x02;
		byte |= (diff0 >> 0) & 0x01;
		byte |= (diff2 << 8) & 0x100;
		dat[byte] ^= (1 << bit);
		return 1;
	}
	diff0 |= (diff1 << 8);
	diff0 |= (diff2 << 16);
	
	if ((diff0 & ~(1 << fls(diff0))) == 0) {
		return 1; /* ecc itself is wrong, data is right */
	}
	
	/* uncorrectable ecc error */
	return -1;
}

static int sprd_nand_correct_data(struct mtd_info *mtd, u_char *dat,
				     u_char *read_ecc, u_char *calc_ecc)
{
	int i, retval = 0;

	if (mtd->writesize > 512) {
		for (i = 0; i < 4; i++) {
			if (correct(dat + 512 * i, read_ecc + 3 * i, calc_ecc + 3 * i) == -1)
				retval = -1;
		}
	} else
		retval = correct(dat, read_ecc, calc_ecc);
	
	return retval;
}

/* driver device registration */
static int sprd_nand_probe(struct platform_device *pdev)
{
	struct nand_chip *this;
	struct sprd_nand_info *info;
	struct sprd_platform_nand *plat = to_nand_plat(pdev);/* get timing */

#ifdef CONFIG_MTD_PARTITIONS
	struct mtd_partition *partitions = NULL;
	int num_partitions = 0;
#endif

	/* set sprd_colrow_addr */
	sprd_colrow_addr.column = 0;
	sprd_colrow_addr.row = 0;
	sprd_colrow_addr.colflag = 0;
	sprd_colrow_addr.rowflag = 0;
	sprd_wr_mode = NO_OP;
	sprd_area_mode = NO_AREA;
	sprd_ecc_mode = NAND_ECC_NONE;

	REG_AHB_CTL0 |= BIT_8 | BIT_9;
	REG_NFC_INTSRC |= BIT_0 | BIT_4 | BIT_5;
	/* 0x1 : WPN disable, and micron nand flash status is 0xeo 
 	   0x0 : WPN enable, and micron nand flash status is 0x60 */
	REG_NFC_WPN = 0x1;
	
	set_gpio_as_nand();
	REG_GR_NFC_MEM_DLY = 0x0;
	set_nfc_param(53);//53MHz
	memset(io_wr_port, 0xff, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);	

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	//info->clk = clk_get(&pdev->dev, "nand"); /* nand clock */
	//clk_enable(info->clk);

	memset(info, 0 , sizeof(*info));
	platform_set_drvdata(pdev, info);/* platform_device.device.driver_data IS info */
	info->platform = plat; /* nand timing */

	sprd_mtd = kmalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	this = (struct nand_chip *)(&sprd_mtd[1]);
	memset((char *)sprd_mtd, 0, sizeof(struct mtd_info));
	memset((char *)this, 0, sizeof(struct nand_chip));

	sprd_mtd->priv = this;
	
	/* set the timing for nand controller */
	sprd_nand_inithw(info, pdev);
	this->cmd_ctrl = sprd_nand_hwcontrol;
	this->dev_ready = sprd_nand_devready;
	this->select_chip = sprd_nand_select_chip;
	this->nfc_readid = sprd_nand_readid;
	this->nfc_wr_oob = sprd_nand_wr_oob;
	this->ecc.calculate = sprd_nand_calculate_ecc;
	this->ecc.correct = sprd_nand_correct_data;
	this->ecc.hwctl = sprd_nand_enable_hwecc;
	this->ecc.mode = NAND_ECC_HW;
	this->ecc.size = 2048;//512;
	this->ecc.bytes = 12;//3
	this->chip_delay = 20;
	this->IO_ADDR_W = this->IO_ADDR_R = io_wr_port;	
	this->priv = info;
	/* scan to find existance of the device */
	nand_scan(sprd_mtd, 1);
	
#ifdef CONFIG_MTD_CMDLINE_PARTS
	sprd_mtd->name = "sprd-nand";
	num_partitions = parse_mtd_partitions(sprd_mtd, part_probes, &partitions, 0);
#endif

	/*printk("num_partitons = %d\n", num_partitions);
	for (i = 0; i < num_partitions; i++) {
		printk("i=%d  name=%s  offset=0x%016Lx  size=0x%016Lx\n", i, partitions[i].name, 
			(unsigned long long)partitions[i].offset, (unsigned long long)partitions[i].size);
	}*/

	if ((!partitions) || (num_partitions == 0)) {
		printk("No parititions defined, or unsupported device.\n");
		goto release;
	}
	add_mtd_partitions(sprd_mtd, partitions, num_partitions);
	return 0;

release:
	nand_release(sprd_mtd);
	return 0;
}

/* device management functions */

static int sprd_nand_remove(struct platform_device *pdev)
{
	struct sprd_nand_info *info;// = to_nand_info(pdev);

	platform_set_drvdata(pdev, NULL);
	if (info == NULL)
		return 0;
	
	del_mtd_partitions(sprd_mtd);
	del_mtd_device(sprd_mtd);
	kfree(sprd_mtd);
	//clk_disable(info->clk);
	kfree(info);	

	return 0;
}

/* PM Support */
#ifdef CONFIG_PM
static int sprd_nand_suspend(struct platform_device *dev, pm_message_t pm)
{
	struct sprd_nand_info *info = platform_get_drvdata(dev);
#if 0
	if (info)
		clk_disable(info->clk);
#endif
	return 0;
}

static int sprd_nand_resume(struct platform_device *dev)
{
	struct sprd_nand_info *info = platform_get_drvdata(dev);

	if (info) {
		//clk_enable(info->clk);
		sprd_nand_inithw(info, dev);
	}

	return 0;
}

#else
#define sprd_nand_suspend NULL
#define sprd_nand_resume NULL
#endif

static struct platform_driver sprd_nand_driver = {
	.probe		= sprd_nand_probe,
	.remove		= sprd_nand_remove,
	.suspend	= sprd_nand_suspend,
	.resume		= sprd_nand_resume,
	.driver		= {
		.name	= "sprd_nand",
		.owner	= THIS_MODULE,
	},
};

static int __init sprd_nand_init(void)
{
	printk("\nSpreadtrum 8800 NAND Driver, (c) 2010 Spreadtrum\n");
	return platform_driver_register(&sprd_nand_driver);
}

static void __exit sprd_nand_exit(void)
{
	platform_driver_unregister(&sprd_nand_driver);
}

module_init(sprd_nand_init);
module_exit(sprd_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Richard Feng <Richard.Feng@spreadtrum.com>");
MODULE_DESCRIPTION("SPRD 8800 MTD NAND driver");