/*
 *  Copyright (C) 2006 Thecus Technology Corp. 
 *
 *      Written by Y.T. Lee (yt_lee@thecus.com)
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for ICH4 GPIO and LED/Button contrl of Thecus N5200
 * Build 2006072201 Add power button support for ICH4
 * Build 2006072401 Emulate ACPI Event output for /proc/thecus_event
 * Build 2006072402 Add power off support when echo "PWR_OFF 5200" to  /proc/thecus_io
 */

/*
 *	Includes, defines, variables, module parameters, ...
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "ICH4_GPIO.h"

/* Module and version information */
#define GPIO_VERSION "2006072402"
#define GPIO_MODULE_NAME "ICH4/4M GPIO driver"
#define GPIO_DRIVER_NAME   GPIO_MODULE_NAME ", v" GPIO_VERSION

/* internal variables */
static unsigned int GPIO_ADDR, PM_ADDR, LPC_ADDR;
static spinlock_t gpio_lock;	/* Guards the hardware */

//static char gpio_expect_close;
static struct pci_dev *ich4_gpio_pci;
static int GP_DIR[4]; //0: output, 1: input

/* module parameters */
static inline void
superio_outb(int reg, int val)
{
	outb(reg, REG);
	outb(val, VAL);
}

static inline int
superio_inb(int reg)
{
	outb(reg, REG);
	return inb(VAL);
}


#define superio_select(ldn) superio_outb(REG_LD_SEL, ldn)

static inline void
superio_enter(void)
{
	outb(SMSC_CNF_START, REG);
}

static inline void
superio_exit(void)
{
	outb(SMSC_CNF_END, REG);
}


//Check for bit , return false when low
int check_bit(unsigned long val, int bn)
{
  if((val >> bn)&0x1){
    //    printk("Check bit want to return 1\n");
    return 1;
  }
  else{
    //printk("Check bit want to return 0\n");
    return 0;
  }
}

/*
Parameters:
bit_n: bit# to read 
Return value:
1: High
0: Low
-1: Error
*/
int read_gpio(int bit_n)
{
  int ret=0;
  u32 gval=0;
  spin_lock(&gpio_lock);

  if(bit_n<=15){ //check GPE0_STS
    gval=inl(PM_ADDR+GPE0_STS);
    ret=check_bit(gval,bit_n);
  }
  else if(bit_n==26){
    ret=-1;
  }
  else if((bit_n>=16)&&(bit_n<29)){ //check GP_LVL
    gval=inl(GPIO_ADDR+GP_LVL);
    ret=check_bit(gval,bit_n);
  }
  else if((bit_n>=32)&&(bit_n<=44)){ //check GP_LVL2
    gval=inl(GPIO_ADDR+GP_LVL2);
    ret=check_bit(gval,bit_n-32);
    
  }
  else
    ret= -1;
  
  spin_unlock(&gpio_lock);

  //if(ret>=0)
  //printk("Read=0x%08X, bit[%d]=%d\n",gval,bit_n,ret);
  
  return ret;
}


/*
Parameters:
bit_n: bit# to update
val: [0/1] values to update 
Return value:(after set)
1: High 
0: Low
-1: Error
*/
int update_gpio(int bit_n, int val)
{
  int ret=0;
  u32 gval,sval,mask_val;
  mask_val=1;
  sval=val;
  spin_lock(&gpio_lock);

  if(bit_n<=15){ //check GPE0_STS
    gval=inl(PM_ADDR+GPE0_STS);
    mask_val=mask_val<<bit_n;
    sval=sval<<bit_n;
    gval=(gval&~mask_val)|sval;
    outl(gval,PM_ADDR+GPE0_STS);
  }
  else if(bit_n==26){
    ret= -1;
  }
  else if((bit_n>=16)&&(bit_n<29)){ //check GP_LVL
    gval=inl(GPIO_ADDR+GP_LVL);
    mask_val=mask_val<<bit_n;
    sval=sval<<bit_n;
    gval=(gval&~mask_val)|sval;
    outl(gval,GPIO_ADDR+GP_LVL);
  }
  else if((bit_n>=32)&&(bit_n<=44)){ //check GP_LVL2
    gval=inl(GPIO_ADDR+GP_LVL2);
    mask_val=mask_val<<(bit_n-32);
    sval=sval<<(bit_n-32);
    gval=(gval&~mask_val)|sval;
    outl(gval,GPIO_ADDR+GP_LVL2);
       
  }
  else
    ret= -1;
  
  spin_unlock(&gpio_lock);
 
  if(ret>=0){
    ret=read_gpio(bit_n);
//    printk("Read after write, bit[%d]=%d\n",bit_n,ret);
  }
  return ret;
}


/*
 *	Notify system
 */
                                                                                

static int ich4_gpio_notify_sys (struct notifier_block *this, unsigned long code, void *unused)
{
	if (code==SYS_DOWN || code==SYS_HALT) {
	  update_gpio(RST_INT,0);
	  mdelay(100);
	  update_gpio(RST_INT,1);
	}

	return NOTIFY_DONE;
}
static struct notifier_block gpio_notifier_block = {
        ich4_gpio_notify_sys, NULL, 0
};

/*
 *	Kernel Interfaces
 */


// ----------------------------------------------------------
DECLARE_WAIT_QUEUE_HEAD (thecus_event_queue);
#define MESSAGE_LENGTH 80
static char Message[MESSAGE_LENGTH];
#define MY_WORK_QUEUE_NAME "btn_sched" // length must < 10
static void intrpt_routine(void *);
static int module_die = 0; /* set this to 1 for shutdown */
static struct workqueue_struct *my_workqueue;
static struct delayed_work Task;
static DECLARE_DELAYED_WORK(Task, intrpt_routine);


static void intrpt_routine(void *irrelevant)
{
  //static int delay_poweroff=0;
	static int btn_copy=0;
	int val;
  	//unsigned long gpio_bits_l, gpio_bits_h;

	//===============  Combo function(Copy_Button)  =================//
	//======== 	Cycle 1 trigger One_Touch_Copy 		 ========//
	//======== 	Cycle 4 trigger Publish_function	 ========//
	//========= while event is triggered trigger Buzzer =============//
	val=read_gpio(Copy_BTN);
	if(val>=0){
	  if(val==0) {
	    btn_copy++;
	  }
	  else{
	    if(btn_copy >= 1){
	      sprintf(Message,"Copy Button: ON\n");
	      wake_up_interruptible(&thecus_event_queue);
	      btn_copy=0;
	    }
	    btn_copy=0;
	  }
	}
	val=0;
	val=inw(PM_ADDR+PM1_STS);
	if((val!=0xFFFFFFFF)){
	  //seq_printf(m,"Power Button: %s\n",check_bit(val,PWR_BTN_BIT)?"ON":"OFF");
	  if(check_bit(val,PWR_BTN_BIT)){
	    sprintf(Message,"button/power PWRF 0 0\n");
	    wake_up_interruptible(&thecus_event_queue);
	    outw(val,PM_ADDR+PM1_STS); //clear it!
	  }
	}
	/*
	* If cleanup wants us to die
	*/
	if (module_die == 0)
	  queue_delayed_work(my_workqueue, &Task, 70);
}

static ssize_t thecus_read_event (
        struct file             *file,
        char                    __user *buffer,
        size_t                  length,
        loff_t                  *ppos)
{
	int i;
	static int finished = 0;
	//	if ((file->f_flags & O_NONBLOCK)
	//  && (list_empty(&acpi_bus_event_list)))
	//  return_VALUE(-EAGAIN);
		
	if (finished) {
		finished = 0;
		return 0;
	}
	
//    	printk(KERN_DEBUG "process %i (%s) going to sleep\n",
//           current->pid, current->comm);
//    	interruptible_sleep_on(&thecus_event_queue);
//    	printk(KERN_DEBUG "awoken %i (%s)\n", current->pid, current->comm);
	for (i = 0; i < length && Message[i]; i++)
		put_user(Message[i], buffer + i);

	finished = 1;
	*ppos+=i;
	return i;
	//return -EAGAIN;
}

static int thecus_open_event(struct inode *inode, struct file *file)
{
  return 0;
}

static int thecus_close_event(struct inode *inode, struct file *file)
{
  return 0;
}


static struct file_operations proc_thecus_event_operations = {
  .open = thecus_open_event,
  .release = thecus_close_event,
  .read = thecus_read_event,
};
//-------------------------------------------------------------------------

static ssize_t proc_thecus_write(struct file *file, const char __user *buf,
			       size_t length, loff_t *ppos)
{
	char *buffer;
	int  i,x;
	int ret;
	u32 val,val2;
	u32 sval,gval;

	if (!buf || length > PAGE_SIZE)
		return -EINVAL;

	buffer = (char *)__get_free_page(GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	ret = -EFAULT;
	if (copy_from_user(buffer, buf, length))
		goto out;

	ret = -EINVAL;
	if (length < PAGE_SIZE)
		buffer[length] = '\0';
	else if (buffer[PAGE_SIZE-1])
		goto out;

	/*
	 * Usage: echo "Fail 0|1" >/proc/thecus_io
	 * Usage: echo "Busy 0|1" >/proc/thecus_io 
	 * Usage: echo "Copy 0|1" >/proc/thecus_io
	 * 
	 */
	// ---------------------------------------------------------- 
	// SATA FAIL LED for N5200 MB V1.2 by K.D.Hung
	// ---------------------------------------------------------- 
#define SFAILBASE 	38
#define SFAILCMD	"SFAIL"
#define SFAILMAPCMD	"SMAPFAIL"
        if (!strncmp (buffer, SFAILCMD, strlen (SFAILCMD)))
	{
            i = sscanf (buffer + strlen (SFAILCMD), "%d %d\n",&val,&val2);
	    // printk("CMD: %s , %d , %d \n",SFAILCMD , val , val2 );
            if (i==2) //only one input
            {        
	      if(val2==0)
		ret=update_gpio(SFAILBASE + val,LED_OFF);
	      else
		ret=update_gpio(SFAILBASE + val,LED_ON);
	    }
	}
        if (!strncmp (buffer, SFAILMAPCMD, strlen (SFAILMAPCMD)))
	{
	    char mask;
            i = sscanf (buffer + strlen (SFAILMAPCMD), "%d\n",&val);
	    // printk("CMD: %s , %d \n",SFAILMAPCMD , val );
      	    if (i==1) // 2 inputs
      	    {        
		char on_off;
	    	for( mask = 0; mask < 5 ; mask++ )
	    	{
		    on_off = ( (val & 1 << mask) > 0 );

		    printk("led %d : %d\n",mask , on_off);

		    update_gpio(SFAILBASE + mask + 1,on_off);
		}
	    }	
	}
	// ---------------------------------------------------------- 
        if (!strncmp (buffer, "Busy", strlen ("Busy"))){
            i = sscanf (buffer + strlen ("Busy"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      if(val==0)//input 0: want to turn off
		ret=update_gpio(LED_Busy,LED_OFF);
	      else
		ret=update_gpio(LED_Busy,LED_ON);
	      
	    }
	}
	else if(!strncmp (buffer, "Fail", strlen ("Fail"))){
            i = sscanf (buffer + strlen ("Fail"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      if(val==0)//input 0: want to turn off
		ret=update_gpio(LED_Fail,LED_OFF);
	      else
		ret=update_gpio(LED_Fail,LED_ON);
	      
	    }
 	}
	else if(!strncmp (buffer, "Copy", strlen ("Copy"))){
            i = sscanf (buffer + strlen ("Copy"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      if(val==0)//input 0: want to turn off
		ret=update_gpio(LED_Copy,LED_OFF);
	      else
		ret=update_gpio(LED_Copy,LED_ON);
	      
	    }
 	}
	else if(!strncmp (buffer, "Host_boot", strlen ("Host_boot"))){
            i = sscanf (buffer + strlen ("Host_boot"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      if(val==0)//input 0: want to turn off
		ret=update_gpio(RST_INT,0);
	      else
		ret=update_gpio(RST_INT,1);
	      
	    }
 	}
	else if(!strncmp (buffer, "uP_reset", strlen ("uP_reset"))){
            i = sscanf (buffer + strlen ("uP_reset"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      if(val==0)//input 0: want to turn off
		ret=update_gpio(RST_51,0);
	      else
		ret=update_gpio(RST_51,1);
	      
	    }
 	}
	else if(!strncmp (buffer, "PWR_OFF", strlen ("PWR_OFF"))){
	  ret=-1;
            i = sscanf (buffer + strlen ("PWR_OFF"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      if(val==5200){//input magic_word: want to turn off
		gval=0x3c00;
		outw(gval,PM_ADDR+PM1_CNT);
	      }
	      
	    }
 	}
	else if(!strncmp (buffer, "GP25", strlen ("GPXX"))){
            i = sscanf (buffer + strlen ("GPXX"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      x=0;
	      if(GP_DIR[x]==1){
		spin_lock(&gpio_lock);

		GP_DIR[x]=0;
		gval=inl(GPIO_ADDR+GP_IO_SEL);
		printk("GP_IO_SEL=0x%08X\n",gval);
		
		sval=1;
		sval=sval<<GP25;
		sval=~sval;
		gval&=sval;
		printk("GP_IO_SEL set to =0x%08X\n",gval);
		outl(gval,GPIO_ADDR+GP_IO_SEL);
		
		spin_unlock(&gpio_lock);
	      }
		
	      if(val==0)//input 0: want to turn off
		ret=update_gpio(GP25,0);
	      else
		ret=update_gpio(GP25,1);
	      
	    }
 	}
	else if(!strncmp (buffer, "GP27", strlen ("GPXX"))){
            i = sscanf (buffer + strlen ("GPXX"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      x=1;
	      if(GP_DIR[x]==1){
		spin_lock(&gpio_lock);

		GP_DIR[x]=0;
		gval=inl(GPIO_ADDR+GP_IO_SEL);
		printk("GP_IO_SEL=0x%08X\n",gval);
		
		sval=1;
		sval=sval<<GP27;
		sval=~sval;
		gval&=sval;
		printk("GP_IO_SEL set to =0x%08X\n",gval);
		outl(gval,GPIO_ADDR+GP_IO_SEL);
		
		spin_unlock(&gpio_lock);
	      }
	      if(val==0)//input 0: want to turn off
		ret=update_gpio(GP27,0);
	      else
		ret=update_gpio(GP27,1);
	      
	    }
 	}
	else if(!strncmp (buffer, "GP33", strlen ("GPXX"))){
            i = sscanf (buffer + strlen ("GPXX"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      x=2;
	      if(GP_DIR[x]==1){
		spin_lock(&gpio_lock);

		GP_DIR[x]=0;
		gval=inl(GPIO_ADDR+GP_IO_SEL2);
		printk("GP_IO_SEL2=0x%08X\n",gval);
		
		sval=1;
		sval=sval<<(GP33-32);
		sval=~sval;
		gval&=sval;
		printk("GP_IO_SEL2 set to =0x%08X\n",gval);
		outl(gval,GPIO_ADDR+GP_IO_SEL2);
		
		spin_unlock(&gpio_lock);
	      }
	      if(val==0)//input 0: want to turn off
		ret=update_gpio(GP33,0);
	      else
		ret=update_gpio(GP33,1);
	      
	    }
 	}
	else if(!strncmp (buffer, "GP34", strlen ("GPXX"))){
            i = sscanf (buffer + strlen ("GPXX"), "%d\n",&val);
            if (i==1) //only one input
            {        
	      x=3;
	      if(GP_DIR[x]==1){
		spin_lock(&gpio_lock);

		GP_DIR[x]=0;
		gval=inl(GPIO_ADDR+GP_IO_SEL2);
		printk("GP_IO_SEL2=0x%08X\n",gval);
		
		sval=1;
		sval=sval<<(GP34-32);
		sval=~sval;
		gval&=sval;
		printk("GP_IO_SEL2 set to =0x%08X\n",gval);
		outl(gval,GPIO_ADDR+GP_IO_SEL2);
		
		spin_unlock(&gpio_lock);
	      }
	      if(val==0)//input 0: want to turn off
		ret=update_gpio(GP34,0);
	      else
		ret=update_gpio(GP34,1);
	      
	    }
 	}
	else
	  ;
 out:	
	free_page((unsigned long)buffer);
	if(ret>=0)
	  return length;
	else
	  return -EINVAL;
}


static int proc_thecus_show(struct seq_file *m, void *v)
{
  int val,fan_rpm;
  u8 val1,val2;




  val=read_gpio(Copy_BTN);
  if(val>=0)
    seq_printf(m,"Copy button: %s\n", val?"OFF":"ON");


  //  val=read_gpio(DB_DET);
  //if(val>=0)
  //  seq_printf(m,"Switch board: %s\n", val?"No":"Yes");

  val=read_gpio(LED_Copy);
  if(val>=0)
    seq_printf(m,"Copy LED: %s\n", val?"OFF":"ON");

  val=read_gpio(LED_Fail);
  if(val>=0)
    seq_printf(m,"Fail LED: %s\n", val?"OFF":"ON");

  val=read_gpio(LED_Busy);
  if(val>=0)
    seq_printf(m,"Busy LED: %s\n", val?"OFF":"ON");

  val=read_gpio(RST_INT);
  if(val>=0)
    seq_printf(m,"Reset INT: %s\n", val?"High":"Low");
  val=read_gpio(RST_51);
  if(val>=0)
    seq_printf(m,"Reset 51: %s\n", val?"High":"Low");

  val=read_gpio(GP25);
  if(val>=0)
     seq_printf(m,"GPIO_25: %s\n", val?"High":"Low");
  val=read_gpio(GP27);
  if(val>=0)
    seq_printf(m,"GPIO_27: %s\n", val?"High":"Low");
  val=read_gpio(GP33);
  if(val>=0)
    seq_printf(m,"GPIO_33: %s\n", val?"High":"Low");
  val=read_gpio(GP34);
  if(val>=0)
    seq_printf(m,"GPIO_34: %s\n", val?"High":"Low");
  // ------------------------------------------------
  val=read_gpio(GP39);
  if(val>=0)
    seq_printf(m,"GPIO_39: %s\n", val?"High":"Low");

  val=read_gpio(GP40);
  if(val>=0)
    seq_printf(m,"GPIO_40: %s\n", val?"High":"Low");

  val=read_gpio(GP41);
  if(val>=0)
    seq_printf(m,"GPIO_41: %s\n", val?"High":"Low");

  val=read_gpio(GP42);
  if(val>=0)
    seq_printf(m,"GPIO_42: %s\n", val?"High":"Low");

  val=read_gpio(GP43);
  if(val>=0)
    seq_printf(m,"GPIO_43: %s\n", val?"High":"Low");

  val=read_gpio(GP44);
  if(val>=0)
    seq_printf(m,"GPIO_44: %s\n", val?"High":"Low");
  // ------------------------------------------------



  val=inl(PM_ADDR+GPE0_STS);
  //printk("STS(0x%08X)=0x%08X\n",PM_ADDR+GPE0_STS,val);
  if((val!=0xFFFFFFFF)&&(val!=0))
    seq_printf(m,"Switch board: %s\n",check_bit(val,DB_DET+16)?"No":"Yes");


  val1=inb(LPC_ADDR+TACH1_LSB);
  val2=inb(LPC_ADDR+TACH1_MSB);
  //printk("LSB_1=0x%02X\nMSB_1=0x%02X\n",val1,val2);
  fan_rpm=(val2<<8)|val1;
  if(fan_rpm==0xFFFF)
    fan_rpm=0;
  seq_printf(m,"FAN 1 RPM: %d\n",fan_rpm);
  val1=inb(LPC_ADDR+TACH2_LSB);
  val2=inb(LPC_ADDR+TACH2_MSB);
  //printk("LSB_2=0x%02X\nMSB_2=0x%02X\n",val1,val2);
  fan_rpm=(val2<<8)|val1;
  if(fan_rpm==0xFFFF)
    fan_rpm=0;
  seq_printf(m,"FAN 2 RPM: %d\n",fan_rpm);

  return 0;
}

static int proc_thecus_open(struct inode *inode, struct file *file)
{
        return single_open(file, proc_thecus_show, NULL);
}

static struct file_operations proc_thecus_operations = {
        .open           = proc_thecus_open,
        .read           = seq_read,
	.write          = proc_thecus_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};


int thecus_init_procfs(void)
{
	struct proc_dir_entry *pde;
	spin_lock_init(&gpio_lock);
	pde = proc_create("thecus_io", 0, NULL, &proc_thecus_operations);
	if (!pde)
	  return -ENOMEM;
/*	pde->proc_fops = &proc_thecus_operations;*/
	
	my_workqueue = create_workqueue(MY_WORK_QUEUE_NAME);
	queue_delayed_work(my_workqueue, &Task, 100);
	init_waitqueue_head (&thecus_event_queue);
	
	pde = proc_create("thecus_event", S_IRUSR, NULL, &proc_thecus_event_operations);
	if (!pde)
	  return -ENOMEM;
/*	pde->proc_fops = &proc_thecus_event_operations;*/


	return 0;

}

void thecus_exit_procfs(void)
{
	remove_proc_entry("thecus_io", NULL);
	remove_proc_entry("thecus_event", NULL);

	module_die = 1; /* keep intrp_routine from queueing itself */
//	cancel_delayed_work(&Task); /* no "new ones" */
//	flush_workqueue(my_workqueue); /* wait till all "old ones" finished */
//	destroy_workqueue(my_workqueue);

}

/*
 * Data for PCI driver interface
 *
 * This data only exists for exporting the supported
 * PCI ids via MODULE_DEVICE_TABLE.  We do not actually
 * register a pci_driver, because someone else might one day
 * want to register another driver on the same PCI id.
 */
static struct pci_device_id ich4_gpio_pci_tbl[] = {
	{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801DB_0,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801DB_12,	PCI_ANY_ID, PCI_ANY_ID, },
	{ 0, },			/* End of list */
};
MODULE_DEVICE_TABLE (pci, ich4_gpio_pci_tbl);

/*
 *	Init & exit routines
 */

static unsigned char __init ich4_gpio_getdevice (void)
{
	struct pci_dev *dev = NULL;
	u8 val1, val2;
	
	u32 badr;
	u32 gval,sval;
	int i;
	/*
	 *      Find the PCI device
	 */

	while ((dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev)) != NULL) {
		if (pci_match_id(ich4_gpio_pci_tbl, dev)) {
			ich4_gpio_pci = dev;
			break;
		}
	}

	if (ich4_gpio_pci) {
		pci_read_config_byte (ich4_gpio_pci, GPIOBASE, &val1);
		pci_read_config_byte (ich4_gpio_pci, GPIOBASE+1, &val2);
		badr = ((val2 << 2) | (val1 >> 6)) << 6;
		printk("XXX=0x%04X\n",badr);
		//pci_read_config_dword(ich4_gpio_pci, GPIOBASE,&badr);
		GPIO_ADDR = badr;
		if (badr == 0x0001 || badr == 0x0000) {
			printk ("failed to get GPIO_ADDR address\n");
			return 0;
		}
		printk("Found ICH4 GPIO at 0x%08X\n",GPIO_ADDR);
		sval=1;
		sval=~sval;
		GPIO_ADDR&=sval;

		pci_read_config_byte (ich4_gpio_pci, GPIO_CNTL, &val1);
		if(val1==0x10){
		  //pci_write_config_byte (ich4_gpio_pci, GPIO_CNTL, 0);
		  printk("GPIO already turned on\n");
		}
		else{
		  pci_write_config_byte (ich4_gpio_pci, GPIO_CNTL, 0x10);
		  printk("Turn on the GPIO\n");
		}
		
		pci_write_config_word (ich4_gpio_pci, 0xe4, 0x801);


		pci_read_config_byte (ich4_gpio_pci, PMBASE, &val1);
		pci_read_config_byte (ich4_gpio_pci, PMBASE+1, &val2);
		badr = ((val2 << 1) | (val1 >> 7)) << 7;

		PM_ADDR = badr;
		if (badr == 0x0001 || badr == 0x0000) {
			printk ( "failed to get PM_ADDR address\n");
			return 0;
		}
		printk("Found ICH4 PM at 0x%08X\n",PM_ADDR);

		/* GPIO SEL */
		gval=inl(GPIO_ADDR+GPIO_USE_SEL);
		printk("GPIO_USE_SEL=0x%08X\n",gval);
		sval=0x3;
		//sval=~sval;
		gval|=sval;
		printk("GPIO_USE_SEL set to =0x%08X\n",gval);
		outl(gval, GPIO_ADDR+GPIO_USE_SEL);

		gval=inl(GPIO_ADDR+GPIO_USE_SEL2);
		printk("GPIO_USE_SEL2=0x%08X\n",gval);
		gval=0xfff;
		printk("GPIO_USE_SEL2 set to =0x%08X\n",gval);
		outl(gval, GPIO_ADDR+GPIO_USE_SEL2);



		for(i=0;i<4;i++)
		  GP_DIR[i]=1; //set GP25,27,33,34 default to input
		
		/* Set R/W directions 1 for input*/
		gval=inl(GPIO_ADDR+GP_IO_SEL);
		printk("GP_IO_SEL=0x%08X\n",gval);
		
		sval=1;
		sval=sval<<GP25;
		gval|=sval;
		sval=1;
		sval=sval<<GP27;
		gval|=sval;
		printk("GP_IO_SEL set to =0x%08X\n",gval);
		outl(gval,GPIO_ADDR+GP_IO_SEL);

		gval=inl(GPIO_ADDR+GP_IO_SEL2);
		printk("GP_IO_SEL2=0x%08X\n",gval);

		sval=1;
		sval=sval<<(GP33-32);
		gval|=sval;
		sval=1;
		sval=sval<<(GP34-32);
		gval|=sval;
		sval=1;
		sval=sval<<(Copy_BTN-32);
		gval|=sval;

		printk("GP_IO_SEL2 set to =0x%08X\n",gval);
		outl(gval,GPIO_ADDR+GP_IO_SEL2);
		
		  pci_read_config_byte (ich4_gpio_pci, GPIO_PM_CON_2, &val1);
		  pci_read_config_byte (ich4_gpio_pci, GPIO_PM_CON_3, &val2);
		  printk("(1)PM_CON_2=0x%02X\n   PM_CON_3=0x%02X\n",val1,val2);
		  val2=0x0;
		  pci_write_config_byte (ich4_gpio_pci, GPIO_PM_CON_3, val2);
		  pci_read_config_byte (ich4_gpio_pci, GPIO_PM_CON_2, &val1);
		  pci_read_config_byte (ich4_gpio_pci, GPIO_PM_CON_3, &val2);
		  printk("(2)PM_CON_2=0x%02X\n   PM_CON_3=0x%02X\n",val1,val2);
		  
		  printk("Route power button to ICH4 driver\n");
		  gval=0x00;
		  outw(gval,PM_ADDR+PM1_EN);

		  
		return 1;
	}
	return 0;
}


static int smsc47m182_find(int address)
{
	u8 val,val1;

	superio_enter();
	val = superio_inb(SUPERIO_REG_DEVID);

	if (val ==  SMSC_47m182_DEV_ID){
		printk(KERN_INFO "smsc47m1: Found SMSC LPC47M182\n");
		superio_outb(REG_LDNUM,LDNUM_0);
	}
	else {
		superio_exit();
		return -ENODEV;
	}

	superio_select(GPIO_LD);
	address = (superio_inb(SUPERIO_REG_BASE) << 8)
		 |  superio_inb(SUPERIO_REG_BASE + 1);
	val = superio_inb(SUPERIO_REG_ACT);
	if (address == 0 || (val & 0x01) == 0) {
		printk(KERN_INFO "smsc47m1: GPIO Device is disabled, will not use\n");
		superio_exit();
		return -ENODEV;
	}
	printk(KERN_INFO "smsc47m1: GPIO base address=0x%08X\n",address);
	val1=inb((address)+REG_GPIO16);
	printk("GPIO 16=0x%02X\n",val1);
	val1=PME_Enable;
	outb(val1,(address)+REG_GPIO16);
	val1=inb((address)+REG_GPIO17);
	printk("GPIO 17=0x%02X\n",val1);
	val1=PME_Enable;
	outb(val1,(address)+REG_GPIO17);
	

	superio_select(PM_LD);
	address = (superio_inb(SUPERIO_REG_BASE) << 8)
		 |  superio_inb(SUPERIO_REG_BASE + 1);
	val = superio_inb(SUPERIO_REG_ACT);
	if (address == 0 || (val & 0x01) == 0) {
		printk(KERN_INFO "smsc47m1: PM Device is disabled, will not use\n");
		superio_exit();
		return -ENODEV;
	}
	LPC_ADDR=address;
	printk(KERN_INFO "smsc47m1: PM base address=0x%08X\n",address);
	superio_exit();
	return 0;
}


static int __init thecus_init (void)
{
	int ret;
	int temp_addr=0;
	spin_lock_init(&gpio_lock);

	/* Check whether or not the ICH4 LPC is there */
	if (!ich4_gpio_getdevice () || ich4_gpio_pci == NULL)
		return -ENODEV;

 	if (smsc47m182_find(temp_addr)) {
		return -ENODEV;
	}


	if (!request_region (GPIO_ADDR, GPIO_IO_PORTS, "ICH4 GPIO")) {
		printk ("I/O address 0x%04x already in use\n",GPIO_ADDR);
		ret = -EIO;
		goto out;
	}
	if (!request_region (LPC_ADDR, SMSC_EXTENT, "SMSC LPC")) {
		printk ("I/O address 0x%04x already in use\n",LPC_ADDR);
		ret = -EIO;
		goto unreg_region1;
	}
	if (!request_region (ICH4_LPC_SIO,ICH4_LPC_SIO_PORTS,"LPC_SIO")) {
		printk ("I/O address 0x%04x already in use\n",LPC_ADDR);
		ret = -EIO;
		goto unreg_region2;
	}

	ret = register_reboot_notifier(&gpio_notifier_block);
	if (ret != 0) {
		printk("cannot register reboot notifier (err=%d)\n",
			ret);
		goto unreg_region;
	}

	if( thecus_init_procfs()){
	  printk(KERN_ERR "ICH4_GPIO: cannot create /proc/thecus_io.\n");
	  return -ENOENT;
	}
	else{
	  printk(KERN_INFO "%s Loaded\n",GPIO_DRIVER_NAME);
	}


	return 0;


	unregister_reboot_notifier(&gpio_notifier_block);
 unreg_region:
	release_region (LPC_ADDR, SMSC_EXTENT);
 unreg_region1:
	release_region (GPIO_ADDR, GPIO_IO_PORTS);
 unreg_region2:
	release_region (ICH4_LPC_SIO,ICH4_LPC_SIO_PORTS);
out:
	return ret;
}

static void __exit thecus_cleanup (void)
{

	thecus_exit_procfs();
	/* Deregister */
	unregister_reboot_notifier(&gpio_notifier_block);
	release_region (GPIO_ADDR, GPIO_IO_PORTS);
	release_region (LPC_ADDR, SMSC_EXTENT);
	release_region (ICH4_LPC_SIO,ICH4_LPC_SIO_PORTS);
}

module_init(thecus_init);
module_exit(thecus_cleanup);

MODULE_AUTHOR("Y.T. Lee");
MODULE_DESCRIPTION("GPIO Driver for Thecus N5200");
MODULE_LICENSE("GPL");
