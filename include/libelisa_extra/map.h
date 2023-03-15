/*
 *
 * Copyright 2023 Kenichi Yasukata
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _LIBELISA_EXTRA_H
#define _LIBELISA_EXTRA_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <link.h>
#include <libelf.h>
#include <gelf.h>

#include <libelisa.h>

static inline void *elisa_create_program_map_req(const char *program_filename,
						 uint64_t base_gpa,
						 void **handle,
						 struct elisa_map_req **map_req,
						 int *map_req_cnt,
						 int *map_req_num)
{
	*map_req_cnt = 0;
	{
		int fd;
		assert((fd = open(program_filename, O_RDONLY)) != -1);
		{
			Elf *elf;
			elf_version(EV_CURRENT);
			assert((elf = elf_begin(fd, ELF_C_READ_MMAP, NULL)) != NULL);
			{
				size_t phdrnum;
				assert(!elf_getphdrnum(elf, &phdrnum));
				{
					size_t i;
					for (i = 0; i < phdrnum; i++) {
						GElf_Phdr phdr;
						assert(gelf_getphdr(elf, i, &phdr) == &phdr);
						if (phdr.p_type == PT_LOAD) {
							uint64_t j;
							size_t s = (phdr.p_vaddr % 0x1000 + phdr.p_memsz);
							for (j = 0; j < (s % 0x1000 ? s / 0x1000 + 1 : s / 0x1000); j++) {
								if (*map_req_cnt == *map_req_num) {
									if (*map_req_num == 0) {
										assert(*map_req == NULL);
										*map_req_num = 8;
									} else
										*map_req_num *= 2;
									assert((*map_req = (struct elisa_map_req *) realloc(*map_req, sizeof(struct elisa_map_req) * *map_req_num)) != NULL);
								}
								(*map_req)[*map_req_cnt].dst_gpa = 0x1000 * *map_req_cnt + base_gpa;
								(*map_req)[*map_req_cnt].dst_gva = (phdr.p_vaddr & (~((1UL << 12) - 1))) + 0x1000 * j;
								(*map_req)[*map_req_cnt].src_gxa = (phdr.p_vaddr & (~((1UL << 12) - 1))) + 0x1000 * j;
								(*map_req)[*map_req_cnt].flags = 0;
								assert(!((*map_req)[*map_req_cnt].dst_gpa % (1UL << (12 + 9 * (1 /*target_level*/ - 1)))));
								(*map_req)[*map_req_cnt].level = 1;
								(*map_req)[*map_req_cnt].pt_flags = PT_P | PT_W | PT_U;
								(*map_req)[*map_req_cnt].ept_flags = EPT_R | EPT_W | EPT_X | EPT_U | EPT_MT;
								(*map_req_cnt)++;
							}
						}
					}
				}
			}
		}
		close(fd);
	}
	{
		assert(((*handle) = dlmopen(LM_ID_NEWLM, program_filename, RTLD_NOW | RTLD_LOCAL)) != NULL); // we do not close this
		{
			struct link_map *lm;
			assert(!dlinfo(*handle, RTLD_DI_LINKMAP, &lm));
			{
				int i;
				for (i = 0; i < *map_req_cnt; i++) {
					(*map_req)[i].dst_gva += lm->l_addr;
					(*map_req)[i].src_gxa += lm->l_addr;
					assert(!((*map_req)[i].dst_gva % (1UL << (12 + 9 * (1 /*target_level*/ - 1)))));
					assert(!((*map_req)[i].src_gxa % (1UL << (12 + 9 * (1 /*target_level*/ - 1)))));
				}
			}
		}
	}
	return 0;
}

#endif
