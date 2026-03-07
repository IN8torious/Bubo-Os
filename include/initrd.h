#pragma once
#include <stdint.h>
#include "vfs.h"

vfs_node_t* initrd_init(uint64_t addr, uint64_t size);
void        initrd_list(void);
