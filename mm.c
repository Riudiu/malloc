/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/// 가용 리스트 조작을 위한 기본 상수와 매크로 정의
#define WSIZE 4       // Word and header/footer 크기(bytes)
#define DSIZE 8       // Double word 크기 (btyes)
#define CHUNKSIZE   (1 << 12)   // 초기 가용 블록과 힙 확장을 위한 기본 크기

#define MAX(x, y)   ((x) > (y) ? (x) : (y))    // x > y가 참이면 x, 거짓이면 y

// PACK매크로 : 크기와 할당 비트를 통합해서 header와 footer에 저장할 수 있는 값 리턴
#define PACK(size, alloc)   ((size) | (alloc))

// 주소에서 word 읽기 및 쓰기
// unsigned int로 캐스팅하면 음의 정수값을 빼고 할당할 수 있음
#define GET(p)  (*(unsigned int *)(p))        // 인자 p가 참조하는 워드를 읽어서 리턴
#define PUT(p, val)  (*(unsigned int *)(p) = (val))  // 인자 p가 가리키는 워드에 val 저장

// 주소 p에서 크기 및 할당된 필드 읽기
#define GET_SIZE(p)    (GET(p) & ~0x7)  // header or footer의 사이즈 반환(8의 배수)
#define GET_ALLOC(p)   (GET(p) & 0x1)   // 현재 블록 가용 여부 판단(0이면 alloc, 1이면 free)

// bp(현재 블록의 포인터)로 현재 블록의 header 위치와 footer 위치 반환
#define HDRP(bp)    ((char *)(bp) - WSIZE)
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 다음과 이전 블록의 포인터 반환
#define NEXT_BLKP(bp)   (((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE)))    // 다음 블록 bp 위치 반환(bp + 현재 블록의 크기)
#define PREV_BLKP(bp)   (((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE)))    // 이전 블록 bp 위치 반환(bp - 이전 블록의 크기)

/// Declaration
static void *heap_listp;

/* 
 * mm_init - initialize the malloc package.
 * 초기 가용 블록으로 힙 만들기
 */
int mm_init(void)
{   
    // 비어있는 힙 생성
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) {  
        // heap_listp가 힙의 최댓값 이상을 요청한다면 fail
        return -1;
    }

    PUT(heap_listp, 0);                             // 선형 padding
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    // 프롤로그 header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    // 프롤로그 footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));        // 에필로그 header
    heap_listp += (2*WSIZE);

    // CHUNKSIZE 바이트의 사용 가능한 블록으로 빈 힙 확장
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

/*
 * extend_heap 사용 경우
 * 1) 힙이 초기화될 때
 * 2) mm_malloc이 적당한 fit을 찾지 못했을 때
 */
void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;

    // 짝수 단어를 할당하여 정렬 상태 유지
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // words가 홀수면 +1을 해서 공간 할당
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    // 사용 가능한 블록 header/footer 및 epilogue header 초기화
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    /*
      extend_heap 블록 너머에 오지 않도록 배치한 블록 다음 공간을 블록이라 가정하고 epilogue header 배치
      (실제로는 존재하지 않는 블록)
    */

    // 이전 블록이 사용 가능한 경우 병합
    return coalesce(bp);   
}

void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // case1: 앞, 뒤 블록 모두 할당되어 있을 때
    if (prev_alloc && next_alloc) {
        return bp;
    }

    // case2: 앞 블록 할당, 뒷 블록 가용
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // case3: 앞 블록 가용, 뒷 블록 할당
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    // case4: 앞, 뒤 블록 모두 가용
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
} 

void *find_fit(size_t a_size) {
    void *bp;

    for (bp = (char *)heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (a_size <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL;    // NO fit
}

/* 
 * mm_malloc - 새 가용 블록으로 힙 확장하기 
 * brk 포인터를 증가시켜 블록을 할당, 항상 크기가 선형의 배수인 블록을 할당합니다.
 */
void *mm_malloc(size_t size) {
    size_t a_size;       // 조정된 block 크기
    size_t extend_size;  // 적합하지 않은 경우 힙을 확장할 크기
    char *bp;

    // 가짜 요청 무시
    if (size == 0) {
        return NULL;
    }

    // 오버헤드 및 정렬 요청을 포함하도록 블록 크기 조정
    if (size <= DSIZE) {    
        // 2words 이하의 사이즈는 4워드로 할당 요청 (header 1word, footer 1word)
        a_size = 2*DSIZE;
    }
    else {                  
        // 할당 요청의 용량이 2words 초과 시, 충분한 8byte의 배수의 용량 할당
        a_size = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }

    // Search the free list for a fit
    if ((bp = find_fit(a_size)) != NULL) {   // 적당한 크기의 가용 블록 검색
        place(bp, a_size);                   // 초과 부분을 분할하고 새롭게 할당한 블록의 포인터 반환
        return bp;
    }

    // NO fit found, 더 많은 메모리를 확보하고 블록 배치
    extend_size = MAX(a_size, CHUNKSIZE);
    if ((bp = extend_heap(extend_size/WSIZE)) == NULL) {    // 칸의 개수
        return NULL;
    }
    place(bp, a_size);
    return bp;
}

void place(void *bp, size_t a_size) {
    size_t c_size = GET_SIZE(HDRP(bp));

    if ((c_size - a_size) >= (2 * (DSIZE))) {
        // 요청 용량 만큼 블록 배치
        PUT(HDRP(bp), PACK(a_size, 1));
        PUT(FTRP(bp), PACK(a_size, 1));
        
        bp = NEXT_BLKP(bp);
        // 남은 블록에 header, footer 배치
        PUT(HDRP(bp), PACK(c_size - a_size, 0));
        PUT(FTRP(bp), PACK(c_size - a_size, 0));
    }
    else {      
        // csize와 aszie 차이가 네 칸(16byte)보다 작다면 해당 블록 통째로 사용
        PUT(HDRP(bp), PACK(c_size, 1));
        PUT(FTRP(bp), PACK(c_size, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    void *old_bp = bp;
    void *new_bp;
    size_t copySize;
    
    new_bp = mm_malloc(size);
    if (new_bp == NULL)
      return NULL;

    copySize = GET_SIZE(HDRP(old_bp));
    if (size < copySize)
      copySize = size;

    // 메모리의 특정한 부분으로부터 얼마까지의 부분을 다른 메모리 영역으로 복사해주는 함수(old_bp로부터 copySize만큼의 문자를 new_bp로 복사)
    memcpy(new_bp, old_bp, copySize);  
    mm_free(old_bp);
    return new_bp;
}
