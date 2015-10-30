/**
  * @file fmsr.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Contains prototypes for functions implemented in fmsr.c,
  *        main include file accessed by third-party applications.
  * **/

/* ===================================================================
Copyright (c) 2013, Henry C. H. Chen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  - Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  - Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

  - Neither the name of the Chinese University of Hong Kong nor the
    names of its contributors may be used to endorse or promote
    products derived from this software without specific prior written
    permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=================================================================== */


#ifndef LIBFMSR_FMSR_H
#define LIBFMSR_FMSR_H

#include <stdlib.h>

#ifndef LIBFMSR_GF
#define LIBFMSR_GF
/** GF(2^8) symbol, also used to represent various k's and n's which must be
 *  limited to [0, 256).
 * */
typedef unsigned char gf;
#endif

/** struct to hold hints from user to speed up repair process.
 *  @see fmsr_repair() */
typedef struct
{
  gf last_repaired;  /**< previously repaired node, 255 for none */
  gf last_used;      /**< chunk index selected in previous repair for each node (0 or 1) */
} fmsr_repair_hints;


/*  ----------------------------------------------------  */
/* | initialization (call first before doing anything!) | */
/*  ----------------------------------------------------  */
/** Must be called first before using any core functions in the library. */
void fmsr_init(void);


/*  ---------------------------------------------------------------  */
/* | helper functions (e.g., for memory allocation in application) | */
/*  ---------------------------------------------------------------  */
/** Returns id of the node where the index-th chunk resides,
 *  or 255 on failure. */
gf fmsr_nodeid(gf k, gf n, gf index);

/** Returns the number of chunks per node, or 255 on failure. */
gf fmsr_chunks_per_node(gf k, gf n);

/** Returns the indices of chunks on a certain node through chunk_indices,
 *  or 255 on failure. */
gf fmsr_chunks_on_node(gf k, gf n, gf node, gf *chunk_indices);

/** Returns the number of native chunks. */
gf fmsr_nn(gf k, gf n);

/** Returns the number of code chunks. */
gf fmsr_nc(gf k, gf n);

/** Returns the size of input file after being padded. */
size_t fmsr_padded_size(gf k, gf n, size_t size);

// Examples of how much memory to allocate (WITHOUT error-checking ...)
// Remember, these are just examples.
#define alloc_encode_matrix(k, n) (gf *)malloc(fmsr_nc(k, n) * fmsr_nn(k, n))
#define alloc_repair_matrix(k, n) (gf *)malloc(fmsr_chunks_per_node(k, n) * (n-1))

#define alloc_native_chunks(k, n, filesize) \
    (gf *)malloc(fmsr_padded_size(k, n, filesize))
#define alloc_code_chunks(k, n, filesize) \
    (gf *)malloc(fmsr_nc(k, n) * fmsr_padded_size(k, n, filesize)/fmsr_nn(k, n))

#define alloc_decode_chunk_indices(k, n) \
    (gf *)malloc(fmsr_nn(k, n))
#define alloc_decode_chunks(k, n, chunksize) \
    (gf *)malloc(fmsr_nn(k, n) * chunksize)

#define alloc_chunks_to_retrieve(k, n) \
    (gf *)malloc(n-1)
#define alloc_retrieved_chunks(k, n, chunksize) \
    (gf *)malloc((n-1) * chunksize)
#define alloc_new_code_chunks(k, n, chunksize) \
    (gf *)malloc(fmsr_chunks_per_node(k, n) * chunksize)


/*  ----------------  */
/* | core functions | */
/*  ----------------  */

/** Splits file into native chunks and encodes into code chunks.
 *  Note: sufficient memory should be allocated for the padded data (using
 *  fmsr_padded_size()), which is longer than data_size.
 *
 *  @param[in]               k,n (n,k)-FMSR
 *  @param[in]              data original unpadded file to be encoded
 *  @param[in]         data_size unpadded file size
 *  @param[in]        create_new if 0, use supplied encode_matrix;
 *                               else generate and update encode_matrix
 *  @param[out]      code_chunks code chunks from encoded data
 *  @param[in,out] encode_matrix encoding matrix
 *
 *  @return 0 on success; -1 on failure
 * */
int fmsr_encode(gf k, gf n, gf *data, size_t data_size, int create_new,
                gf *code_chunks, gf *encode_matrix);


/** Decodes code chunks to give the original data.
 *
 *  @param[in]               k,n (n,k)-FMSR
 *  @param[in]       code_chunks retrieved code chunks
 *  @param[in]        chunk_size size of each code chunk
 *  @param[in]     chunk_indices indices of retrieved chunks
 *  @param[in]        num_chunks number of chunks retrieved
 *  @param[in]     encode_matrix original encoding matrix
 *                               (can be NULL if decode_matrix is already supplied)
 *  @param[in,out] decode_matrix decoding matrix (can be NULL if not interested)
 *  @param[in]        create_new if 0, use supplied decode_matrix;
 *                               else calculate and update decode_matrix
 *  @param[out]             data decoded data (concat native chunks and unpad)
 *  @param[out]        data_size size of decoded data
 *
 *  @return 0 on success; -1 on failure
 * */
int fmsr_decode(gf k, gf n, gf *code_chunks, size_t chunk_size,
                gf *chunk_indices, gf num_chunks, gf *encode_matrix,
                gf *decode_matrix, int create_new,
                gf *data, size_t *data_size);


/** Informs caller of the chunks and encoding matrix used in repair.
 *  Most of the work during repair is done here.
 *
 *  @param[in]                     k,n (n,k)-FMSR
 *  @param[in]           encode_matrix current encoding matrix before repair
 *  @param[in]                erasures list of failed node indices
 *  @param[in]            num_erasures number of failed nodes
 *  @param[in]                   hints refer to struct fmsr_repair_hints
 *  @param[out]      new_encode_matrix new encoding matrix after repair
 *  @param[out]          repair_matrix repair matrix (follows order given by chunk list below)
 *  @param[out]     chunks_to_retrieve list of code chunks to retrieve from surviving nodes
 *  @param[out] num_chunks_to_retrieve number of code chunks to retrieve
 *
 *  @return number of rounds checked on success (>0); -1 on unsupported parameters;
 *  @return 0 on failing to generate suitable coefficients
 * */
int fmsr_repair(gf k, gf n, gf *encode_matrix,
                gf *erasures, gf num_erasures, fmsr_repair_hints *hints,
                gf *new_encode_matrix, gf *repair_matrix,
                gf *chunks_to_retrieve, gf *num_chunks_to_retrieve);


/** Generates new code_chunks from existing code_chunks.
 *  This is simply a single matrix multiplication that should be called after
 *  fmsr_repair().
 *
 *  @param[in]     repair_matrix repair matrix obtained from fmsr_repair()
 *  @param[in]         rows,cols number of rows and columns in repair_matrix
 *  @param[in]  retrieved_chunks all code chunks retrieved for the repair
 *  @param[in]        chunk_size size of each code chunk
 *  @param[out]  new_code_chunks all code chunks newly generated by this function
 * */
void fmsr_regenerate(gf *repair_matrix, gf rows, gf cols,
                     gf *retrieved_chunks, size_t chunk_size,
                     gf *new_code_chunks);


#endif  /* LIBFMSR_FMSR_H */

