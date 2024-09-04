#pragma once

namespace Thread {
	
	
	struct ThreadConfig {
		seL4_CPtr root_cnode = nullptr;
		//seL4_CPtr root_cnode = nullptr;
	};
	
	class Thread {
		Thread(const ThreadConfig& thread_config);
		
		
		
	};
	
	
	
}