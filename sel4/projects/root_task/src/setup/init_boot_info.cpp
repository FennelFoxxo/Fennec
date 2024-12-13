#include "setup.h"

#include "globals/globals.h"

extern "C" {
#include <sel4platsupport/bootinfo.h>
}

bool Setup::initBootInfo() {
    Globals::boot_info = platsupport_get_bootinfo();
    
    Globals::num_empty_slots = Globals::boot_info->empty.end - Globals::boot_info->empty.start;
	retErrorIfFail(Globals::num_empty_slots >= (int)Globals::BootstrapSlots::end, "Not enough empty slots");
	
	Globals::bootstrap_empty_start = Globals::boot_info->empty.start;
    
    seL4_Word extra_length = Globals::boot_info->extraLen;
    
    seL4_Word header_addr = (seL4_Word)Globals::boot_info + seL4_BootInfoFrameSize;
    
    if (Globals::boot_info->extraLen == 0) {
        return true;
    }
    
    while (true) {
        seL4_BootInfoHeader* header = (seL4_BootInfoHeader*)header_addr;
        seL4_Word id = header->id;
        seL4_Word total_length = header->len;
        void* header_data = (void*)(header_addr + sizeof(seL4_BootInfoHeader));
        
        switch (id) {
            case SEL4_BOOTINFO_HEADER_PADDING:
                goto done;
            case SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER:
                Globals::framebuffer_info = (Globals::MultibootFrameBuffer*)header_data;
                break;
        }
        
        header_addr += total_length;
        extra_length -= total_length;
    }
    
    done:
    
    return true;
}