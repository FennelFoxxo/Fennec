#include "setup.h"

#include "globals/globals.h"
#include "assert.h"

extern "C" {
#include <sel4platsupport/bootinfo.h>
}

void Setup::initBootInfo() {
    Globals::boot_info = platsupport_get_bootinfo();
    
    Globals::num_empty_slots = Globals::boot_info->empty.end - Globals::boot_info->empty.start;
    
    // Make sure there's enough empty slots
	assert(Globals::num_empty_slots >= (int)Globals::BootstrapSlots::end);
	
	Globals::bootstrap_empty_start = Globals::boot_info->empty.start;
    
    seL4_Word extra_length = Globals::boot_info->extraLen;
    
    seL4_Word header_addr = (seL4_Word)Globals::boot_info + seL4_BootInfoFrameSize;
    
    // Get extra information from boot info ex. framebuffer
    if (Globals::boot_info->extraLen == 0) {
        return;
    }
    
    while (true) {
        seL4_BootInfoHeader* header = (seL4_BootInfoHeader*)header_addr;
        seL4_Word id = header->id;
        seL4_Word total_length = header->len;
        void* header_data = (void*)(header_addr + sizeof(seL4_BootInfoHeader));
        
        switch (id) {
            case SEL4_BOOTINFO_HEADER_PADDING:
                return;
            case SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER:
                Globals::framebuffer_info = (Globals::MultibootFrameBuffer*)header_data;
                break;
        }
        
        header_addr += total_length;
        extra_length -= total_length;
    }

}