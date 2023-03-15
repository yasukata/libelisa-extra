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

#ifndef _LIBELISA_EXTRA_IRQ_H
#define _LIBELISA_EXTRA_IRQ_H

#include <libelisa.h>

static inline void elisa_disable_irq_if_enabled(void)
{
	long rflags;
	asm volatile (
		"pushfq \n\t"
		"popq %[rflags] \n\t"
		: [rflags] "=r" (rflags)
		:
		: "rax"
	);
	if (rflags & (1UL << 9))
		vmcall_cli();
}

#endif
