#ifndef _VM_INTERPOSE_H_
#define _VM_INTERPOSE_H_

#define MACHGATE_VM_INTERPOSE_BASENAME "libmachgate_vm_interpose.so"

int machgate_ensure_vm_interpose(char** argv, char** envp);

#endif /* _VM_INTERPOSE_H_ */
