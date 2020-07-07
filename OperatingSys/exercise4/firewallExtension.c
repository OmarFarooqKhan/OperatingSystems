#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <net/tcp.h>
#include <linux/namei.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Extensions to the firewall") ;
MODULE_LICENSE("GPL");


#define _READ 'L'
#define _WRITE 'W'
#define PROC_ENTRY_FILENAME "firewallExtension"


DEFINE_MUTEX(fileLock); //lock for the proc file, only one user can access the lock
static int File_Open = 0; //Prevent multiple file access  


/* make IP4-addresses readable */

#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]


struct nf_hook_ops *reg;
static struct proc_dir_entry *Our_Proc_File;
DECLARE_RWSEM(list_sem);

struct lList {
    unsigned long int port;
    char *dir;
    struct lList *next;
};

struct lList *kernList = NULL; /* the global list of words */


void addEnt(struct lList** list, char *port, char *dir) {
    struct lList *newEnt;
    struct lList *cursor;

    newEnt = kmalloc (sizeof (struct lList), GFP_KERNEL);

    
    kstrtoul(port,0,&newEnt->port);
    kfree(port);
    //newEnt->  port = port;
    newEnt->  dir = dir;
    newEnt -> next =NULL;
	
    if(*list == NULL){
	*list = newEnt;
    }
    else{
    cursor = *list;
	while(cursor->next != NULL){
	 cursor = cursor-> next;
	}
	cursor -> next = newEnt;
    }
}

void show_table (struct lList *sent) {
    
    struct lList *tmp;
    tmp = sent;
    int count = 0;

    while (tmp) {
	count++;
	printk (KERN_INFO "Firewall rule: %d %s\n", tmp->port,tmp->dir);

        tmp = tmp->next;
    }

}

int kernelRead(
			struct file *file,
			char *buffer,
			size_t buffer_size,
			loff_t *offsett){
    char buff[buffer_size];
    
    printk (KERN_INFO "firewall entered L\n");
 
    /* copy data from user space */

    
    if(copy_from_user(buff, buffer, buffer_size)) {
        return -EFAULT;
    }

	
	show_table(kernList);

	int retval =0;
	return retval;

}

/* This function reads in data from the user into the kernel */
ssize_t kernelWrite (struct file *file, const char __user *buffer, size_t count, loff_t *offset) {

    int wrtn = 0; //number of rules written 


    char buff[count];

    
    printk (KERN_INFO "firewall entered\n");

    /* copy data from user space */

    
    if(copy_from_user(buff, buffer, count)) {
        return -EFAULT;
    }


        int crntLineChar = 0;

	char *port;	
	int inLine = 0;
	int space = 0;

	int i;
	for (i=0;i<=count;i++){

		crntLineChar++;

		if(!inLine){

			inLine = 1;
		}

		if (buff[i] == ' '){
			space = i;
			port = (char *) kmalloc(sizeof(char) *crntLineChar,GFP_KERNEL);
			strncpy(port,&buff[i-crntLineChar+1], sizeof(char)* crntLineChar);
			port[crntLineChar-1] = '\0'; //adding null pointer	
		}


		if (buff[i] == '\n'){

			
			int sizep = i - space +1;
			char *dir = (char *)kmalloc(sizeof(char)*sizep,GFP_KERNEL);
			strncpy(dir,&buff[space+1],sizeof(char)*sizep);
			dir[sizep-2] = '\0';
			//printk(KERN_INFO "Firewall rule: %s %s\n",port,dir);


			addEnt(&kernList,port,dir);

			

			crntLineChar = 0;
			inLine = 0;
		}

	}


		
  return wrtn;

}
  

/*
 * The file is opened - we don't really care about
 * that, but it does mean we need to increment the
 * module's reference count.
 */


// the firewall hook - called for each outgoing packet
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 3, 0)
#error "Kernel version < 4.4 not supported!"
//kernels < 4.4 need another firewallhook!
#endif
unsigned int FirewallExtensionHook (void *priv,
                    struct sk_buff *skb,
                    const struct nf_hook_state *state) {

    struct tcphdr *tcp;
    struct tcphdr _tcph;
    struct sock *sk;
    struct mm_struct *mm;
  
  sk = skb->sk;
  if (!sk) {
    printk (KERN_INFO "firewall: netfilter called with empty socket!\n");;
    return NF_ACCEPT;
  }

  if (sk->sk_protocol != IPPROTO_TCP) {
    printk (KERN_INFO "firewall: netfilter called with non-TCP-packet.\n");
    return NF_ACCEPT;
  }
    /* get the tcp-header for the packet */
    tcp = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(struct tcphdr), &_tcph);
    if (!tcp) {
    printk (KERN_INFO "Could not get tcp-header!\n");
    return NF_ACCEPT;
    }
    if (tcp->syn) {
    struct iphdr *ip;
    
    printk (KERN_INFO "firewall: Starting connection \n");
    ip = ip_hdr (skb);
    if (!ip) {
        printk (KERN_INFO "firewall: Cannot get IP header!\n!");
    }
    else {
        printk (KERN_INFO "firewall: Destination address = %u.%u.%u.%u\n", NIPQUAD(ip->daddr));
    }
    printk (KERN_INFO "firewall: destination port = %d\n", ntohs(tcp->dest));
   
    if (in_irq() || in_softirq() || !(mm = get_task_mm(current))) {
        printk (KERN_INFO "Not in user context - retry packet\n");
        return NF_ACCEPT;
    }
    mmput(mm);

        //Find executable here
 	struct path path;
    	pid_t mod_pid;
    	struct dentry *procDentry;
    	struct dentry *parent;

    	char cmdlineFile[80];
    	int res;
    
    	/* current is pre-defined pointer to task structure of currently running task */
    	mod_pid = current->pid;
    	snprintf (cmdlineFile, 80, "/proc/%d/exe", mod_pid); 
    	res = kern_path (cmdlineFile, LOOKUP_FOLLOW, &path);
    	if (res) {
		printk (KERN_INFO "Could not get dentry for %s!\n", cmdlineFile);
		return NF_ACCEPT;
    	}
    
    	procDentry = path.dentry;
    	parent = procDentry->d_parent;
	
	//Build full path

    	printk (KERN_INFO "The name is %s\n", procDentry->d_name.name);
    	printk (KERN_INFO "The name of the parent is %s\n", parent->d_name.name);
    	path_put(&path);

	//getting full raw path of dentry and placing it into buffer
	
	char fullpath[200];
	char *path_file = dentry_path_raw(procDentry,fullpath,200); //full path found here	
	printk (KERN_INFO "full path: %s\n",path_file);

	       
    struct lList *tmp;
    tmp = kernList;

    int portfound = 0;

    while(tmp){
		if (tmp == NULL){
			break;
		}
		if(tmp->port == ntohs(tcp->dest)){
			portfound = 1;
			if(strcmp(tmp->dir,path_file)==0){	
				portfound = 1;
				return NF_ACCEPT;
			}
		}
		tmp = tmp->next;
	}


    if (portfound){
	//port found but not in program list, stop
	tcp_done(sk);
	 printk (KERN_INFO "Connection shut down\n");
	return NF_DROP;
    }

    }


    return NF_ACCEPT;
}

static struct nf_hook_ops firewallExtension_ops = {
    .hook    = FirewallExtensionHook,
    .pf      = PF_INET,
    .priority = NF_IP_PRI_FIRST,
    .hooknum = NF_INET_LOCAL_OUT
};

/* 
 * The file is opened
 */
int procfs_open(struct inode *inode, struct file *file)
{
	mutex_lock(&fileLock);
	if(File_Open){
		mutex_unlock(&fileLock); //release lock
		return -EAGAIN; //proccess currently using proc file
	}
	File_Open++; //increase number of file open (maintaining only one wrt at a time)
   	mutex_unlock(&fileLock);
   	try_module_get(THIS_MODULE);
    	return 0;
}

/* 
 * The file is closed
 */
int procfs_close(struct inode *inode, struct file *file)
{
	mutex_lock(&fileLock);
	File_Open--; // Decrement usage count
   	mutex_unlock(&fileLock);
    	module_put(THIS_MODULE);
    	return 0;		/* success */
}

const struct file_operations File_Ops_4_Our_Proc_File = {
    .owner = THIS_MODULE,
    .write      = kernelWrite,
	.read 		= kernelRead,
    .open      = procfs_open,
    .release = procfs_close,
};
int init_module(void)
{

  int errno;
    
    
  Our_Proc_File =  proc_create_data (PROC_ENTRY_FILENAME, 0644, NULL, &File_Ops_4_Our_Proc_File, NULL);
  /* check if the /proc file was created successfuly */
  if (Our_Proc_File == NULL){
    printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", PROC_ENTRY_FILENAME);
    return -ENOMEM;
  }
  
    printk(KERN_INFO "/proc/%s created\n", PROC_ENTRY_FILENAME);


  errno = nf_register_hook (&firewallExtension_ops); /* register the hook */
  if (errno) {
    printk (KERN_INFO "Firewall extension could not be registered!\n");
  }
  else {
    printk(KERN_INFO "Firewall extensions module loaded\n");
  }

  // A non 0 return means init_module failed; module can't be loaded.
  return errno;
}


void cleanup_module(void)
{
    remove_proc_entry(PROC_ENTRY_FILENAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROC_ENTRY_FILENAME);
    nf_unregister_hook (&firewallExtension_ops); /* restore everything to normal */
    printk(KERN_INFO "Firewall extensions module unloaded\n");
}
