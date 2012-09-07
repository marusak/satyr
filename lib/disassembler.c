/*
    disassembler.c

    Copyright (C) 2011, 2012  Red Hat, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "disassembler.h"
#include "utils.h"
#include "strbuf.h"
#include <string.h>

/**
 * Captures disassembler output into a strbuf.  This is used as a hook
 * in init_disassemble_info() of libopcodes, which is called from
 * btp_disasm_init().
 */
static int
buffer_printf(void *buffer, const char *fmt, ...)
{
    struct btp_strbuf *strbuf = buffer;
    va_list p;
    int orig_len = strbuf->len;

    va_start(p, fmt);
    buffer = btp_strbuf_append_strfv(buffer, fmt, p);
    va_end(p);

    return (strbuf->len - orig_len);
}

struct btp_disasm_state *
btp_disasm_init(const char *file_name,
                char **error_message)
{
    struct btp_disasm_state *state =
        btp_malloc(sizeof(struct btp_disasm_state));

    state->bfd_file = bfd_openr(file_name, NULL);
    if (!state->bfd_file)
    {
        *error_message = btp_asprintf("Failed to open file %s: %s",
                                      file_name,
                                      bfd_errmsg(bfd_get_error()));
        free(state);
        return NULL;
    }

    if (!bfd_check_format(state->bfd_file, bfd_object))
    {
        *error_message = btp_asprintf("Invalid file format of %s: %s",
                                      file_name,
                                      bfd_errmsg(bfd_get_error()));
        bfd_close(state->bfd_file);
        free(state);
        return NULL;
    }

    asection *section =
        bfd_get_section_by_name(state->bfd_file, ".text");

    if (!section)
    {
        *error_message = btp_asprintf(
            "Failed to find .text section in %s: %s",
            file_name,
            bfd_errmsg(bfd_get_error()));

        bfd_close(state->bfd_file);
        free(state);
        return NULL;
    }

    state->disassembler = disassembler(state->bfd_file);
    if (!state->disassembler)
    {
        *error_message = btp_asprintf(
            "Unable to find disassembler for %s",
            file_name);

        bfd_close(state->bfd_file);
        free(state);
        return NULL;
    }

    init_disassemble_info(&state->info, NULL, buffer_printf);
    state->info.arch = bfd_get_arch(state->bfd_file);
    state->info.mach = bfd_get_mach(state->bfd_file);
    state->info.buffer_vma = section->vma;
    state->info.buffer_length = section->size;
    state->info.section = section;
    /* TODO: memory error func */
    bfd_malloc_and_get_section(state->bfd_file, section,
                               &state->info.buffer);

    disassemble_init_for_target(&state->info);
    return state;
}

void
btp_disasm_free(struct btp_disasm_state *state)
{
    if (!state)
        return;

    bfd_close(state->bfd_file);
    free(state->error_message);
    free(state);
}

char **
btp_disasm_get_function_instructions(struct btp_disasm_state *state,
                                     uint64_t start_offset,
                                     uint64_t size,
                                     char **error_message)
{
    asection *section = state->info.section;
    if (start_offset < section->vma
        || (start_offset + size) > section->vma + section->size)
    {
        *error_message = btp_asprintf(
            "Invalid function range: 0x%"PRIx64" - 0x%"PRIx64,
            start_offset,
            start_offset + size);

        return NULL;
    }

    size_t result_capacity = 1000;
    size_t result_count = 0;
    char **result = malloc(result_capacity * sizeof(char*));

    bfd_vma pc = start_offset;
    while (pc < start_offset + size)
    {
        state->info.stream = btp_strbuf_new();
        int count = state->disassembler(pc, &state->info);
        pc += count;
        if (count == 0)
        {
            *error_message = btp_asprintf(
                "Failed to disassemble a part of function on "
                "offset 0x%"PRIx64, pc);
            return NULL;
        }

        /* "+ 2" because we are going to append one instruction and
           then a terminating NULL. */
        if (result_count + 2 == result_capacity)
        {
            result_capacity *= 2;
            result = btp_realloc(result, result_capacity * sizeof(char*));
        }

        result[result_count] = btp_strbuf_free_nobuf(state->info.stream);
        ++result_count;
    }

    result[result_count] = NULL;
    return result;
}

void
btp_disasm_instructions_free(char **instructions)
{
    size_t offset = 0;
    while (instructions[offset])
    {
        free(instructions[offset]);
        ++offset;
    }

    free(instructions);
}

bool
btp_disasm_instruction_is_one_of(char *instruction,
                                 const char **mnemonics)
{
    while (mnemonics)
    {
        size_t len = strlen(*mnemonics);
        if (0 == strncmp(instruction, *mnemonics, len) &&
            (' ' == *(*mnemonics + len) || '\0' == *(*mnemonics + len)))
        {
            return true;
        }

        ++mnemonics;
    }

    return false;
}

bool
btp_disasm_instruction_present(char **instructions,
                               const char **mnemonics)
{
    while (instructions)
    {
        if (btp_disasm_instruction_is_one_of(*instructions,
                                             mnemonics))
        {
            return true;
        }

        ++instructions;
    }

    return false;
}

bool
btp_disasm_instruction_parse_single_address_operand(char *instruction,
                                                    uint64_t *dest)
{
    /* Parse the mnemonic. */
    const char *p = instruction;
    p = btp_skip_non_whitespace(p);
    p = btp_skip_whitespace(p);

    /* Parse the address. */
    int chars_read;
    uintptr_t addr;
    int ret = sscanf(p, "%jx %n", &addr, &chars_read);
    if (ret < 1)
        return false;

    /* check that there is nothing else after the address */
    p += chars_read;
    if(*p != '\0')
        return false;

    if (dest)
        *dest = addr;

    return true;
}

uint64_t *
btp_disasm_get_callee_addresses(char **instructions)
{
    static const char *call_mnems[] = {"call", "callb", "callw",
                                       "calll", "callq", NULL};

    /* Determine the upper bound on the number of calls. */
    size_t result_size = 0, instruction_offset = 0;
    while (instructions[instruction_offset])
    {
        char *instruction = instructions[instruction_offset];
        if (btp_disasm_instruction_is_one_of(instruction, call_mnems))
        {
            uint64_t address;
            if (btp_disasm_instruction_parse_single_address_operand(
                    instruction, &address))
                ++result_size;
        }

        ++instruction_offset;
    }

    /* Create the output array and fill it */
    uint64_t *result = malloc(result_size * sizeof(uint64_t) + 1);
    size_t result_offset = 0;
    instruction_offset = 0;
    while (instructions[instruction_offset])
    {
        char *instruction = instructions[instruction_offset];
        if (btp_disasm_instruction_is_one_of(instruction, call_mnems))
        {
            uint64_t address;
            if (btp_disasm_instruction_parse_single_address_operand(instruction, &address))
            {
                /* Check if address is already stored in the list. */
                size_t result_loop = 0;
                for (; result_loop < result_offset; ++result_loop)
                {
                    if (result[result_loop] == address)
                        break;
                }

                /* If the address is not present in the list, store it
                 * there.
                 */
                if (result_loop == result_offset)
                    result[result_offset++] = address;
            }
        }

        ++instruction_offset;
    }

    result[result_offset] = 0;
    return result;
}