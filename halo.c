#include <err.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

extern uint8_t _binary_memdata_bios_bin_start[];
extern uint8_t _binary_memdata_bios_bin_end[];
extern uint8_t _binary_memdata_kernel_bin_start[];
extern uint8_t _binary_memdata_kernel_bin_end[];
extern uint8_t _binary_memdata_user_bin_start[];
extern uint8_t _binary_memdata_user_bin_end[];

int main(void)
{
    int kvm, vmfd, vcpufd, ret;
    struct kvm_sregs sregs;
    size_t mmap_size;
    struct kvm_run *run;

    kvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvm == -1)
        err(1, "/dev/kvm");

    /* Make sure we have the stable version of the API */
    ret = ioctl(kvm, KVM_GET_API_VERSION, NULL);
    if (ret == -1)
        err(1, "KVM_GET_API_VERSION");
    if (ret != 12)
        errx(1, "KVM_GET_API_VERSION %d, expected 12", ret);

    vmfd = ioctl(kvm, KVM_CREATE_VM, (unsigned long)0);
    if (vmfd == -1)
        err(1, "KVM_CREATE_VM");


#define ROUND_UP(n, v) ((n) - 1 + (v) - ((n) - 1) % (v))

    /* Map it to the second page frame (to avoid the real-mode IDT at 0). */
    struct kvm_userspace_memory_region region;
    int slot = 0;
    uint64_t physaddr = 0x1000;
    uint64_t userspace_addr;

    region.flags = 0;

    for (userspace_addr = (uint64_t)_binary_memdata_bios_bin_start;
	 userspace_addr < (uint64_t)_binary_memdata_bios_bin_end;
	 slot += 1, physaddr += 4096, userspace_addr += 4096) {
	region.slot = slot;
	region.guest_phys_addr = physaddr;
	region.memory_size = 4096;
	region.userspace_addr = userspace_addr;
	printf ("Set map ONE page:Guest PA[0x%llx]->Host VA[0x%llx]\n",
		region.guest_phys_addr, region.userspace_addr);
	ret = ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &region);
	if (ret == -1)
	    err(1, "KVM_SET_USER_MEMORY_REGION");
    }

    for (userspace_addr = (uint64_t)_binary_memdata_kernel_bin_start;
	 userspace_addr < (uint64_t)_binary_memdata_kernel_bin_end;
	 slot += 1, physaddr += 4096, userspace_addr += 4096) {
	region.slot = slot;
	region.guest_phys_addr = physaddr;
	region.memory_size = 4096;
	region.userspace_addr = userspace_addr;
	printf ("Set map ONE page:Guest PA[0x%llx]->Host VA[0x%llx]\n",
		region.guest_phys_addr, region.userspace_addr);
	ret = ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &region);
	if (ret == -1)
	    err(1, "KVM_SET_USER_MEMORY_REGION");
    }

    for (userspace_addr = (uint64_t)_binary_memdata_user_bin_start;
	 userspace_addr < (uint64_t)_binary_memdata_user_bin_end;
	 slot += 1, physaddr += 4096, userspace_addr += 4096) {
	if (userspace_addr == (uint64_t)_binary_memdata_user_bin_start + 1 * 4096)
	    continue;
	region.slot = slot;
	region.guest_phys_addr = physaddr;
	region.memory_size = 4096;
	region.userspace_addr = userspace_addr;
	printf ("Set map ONE page:Guest PA[0x%llx]->Host VA[0x%llx]\n",
		region.guest_phys_addr, region.userspace_addr);
	ret = ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &region);
	if (ret == -1)
	    err(1, "KVM_SET_USER_MEMORY_REGION");
    }


    vcpufd = ioctl(vmfd, KVM_CREATE_VCPU, (unsigned long)0);
    if (vcpufd == -1)
        err(1, "KVM_CREATE_VCPU");

    /* Map the shared kvm_run structure and following data. */
    ret = ioctl(kvm, KVM_GET_VCPU_MMAP_SIZE, NULL);
    if (ret == -1)
        err(1, "KVM_GET_VCPU_MMAP_SIZE");
    mmap_size = ret;
    if (mmap_size < sizeof(*run))
        errx(1, "KVM_GET_VCPU_MMAP_SIZE unexpectedly small");
    run = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpufd, 0);
    if (!run)
        err(1, "mmap vcpu");

    /* Initialize CS to point at 0, via a read-modify-write of sregs. */
    ret = ioctl(vcpufd, KVM_GET_SREGS, &sregs);
    if (ret == -1)
        err(1, "KVM_GET_SREGS");
    sregs.cs.base = 0;
    sregs.cs.selector = 0;
    ret = ioctl(vcpufd, KVM_SET_SREGS, &sregs);
    if (ret == -1)
        err(1, "KVM_SET_SREGS");

    /* Initialize registers: instruction pointer for our code, addends, and
     * initial flags required by x86 architecture. */
    struct kvm_regs regs = {
        .rip = 0x1000,
        .rflags = 0x2,
    };
    ret = ioctl(vcpufd, KVM_SET_REGS, &regs);
    if (ret == -1)
        err(1, "KVM_SET_REGS");

    /* Repeatedly run code and handle VM exits. */
    while (1) {
        ret = ioctl(vcpufd, KVM_RUN, NULL);
        if (ret == -1)
            err(1, "KVM_RUN");
        switch (run->exit_reason) {
	case KVM_EXIT_MMIO:
	    printf ("KVM_EXIT_MMIO: phys_addr[0x%llx]\n", run->mmio.phys_addr);
	    printf ("KVM_EXIT_MMIO: data[%x]\n", *(uint8_t *)run->mmio.data);
	    printf ("KVM_EXIT_MMIO: len[%x]\n", run->mmio.len);
	    printf ("KVM_EXIT_MMIO: is_write[%x]\n", run->mmio.is_write);
	ioctl(vcpufd, KVM_GET_REGS, &regs);
	printf ("now rip[0x%llx]\n", regs.rip);
	printf ("now eax[0x%llx]\n", regs.rax);
	    break;
        case KVM_EXIT_HLT:
            puts("KVM_EXIT_HLT");
	ioctl(vcpufd, KVM_GET_REGS, &regs);
	printf ("now rip[0x%llx]\n", regs.rip);
	printf ("now eax[0x%llx]\n", regs.rax);
            return 0;
        case KVM_EXIT_IO:
            if (run->io.direction == KVM_EXIT_IO_OUT && run->io.size == 1 && run->io.port == 0x3f8 && run->io.count == 1) {
                putchar(*(((char *)run) + run->io.data_offset));
	    }
            else
                errx(1, "unhandled KVM_EXIT_IO");
            break;
        case KVM_EXIT_FAIL_ENTRY:
            errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
                 (unsigned long long)run->fail_entry.hardware_entry_failure_reason);
        case KVM_EXIT_INTERNAL_ERROR:
	ioctl(vcpufd, KVM_GET_REGS, &regs);
	printf ("now rip[0x%llx]\n", regs.rip);
	printf ("now eax[0x%llx]\n", regs.rax);
            errx(1, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x", run->internal.suberror);
        default:
            errx(1, "exit_reason = 0x%x", run->exit_reason);
        }
	ioctl(vcpufd, KVM_GET_REGS, &regs);
//	printf ("now rip[0x%llx]\n", regs.rip);
    }
}
