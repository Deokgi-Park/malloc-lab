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
    "3team",
    /* First member's full name */
    "Park Deokgi",
    /* First member's email address */
    "ejrrl6931@gmail.com",
    /* Second member's full name (leave blank if none) */
    "Kim minjun",
    /* Second member's email address (leave blank if none) */
    "minjun@gmail.com"
};

// 워드 사이즈 4
#define WSIZE 4
// 더블워드 사이즈 8
#define DSIZE 8
// 청크 사이즈 2^12을 표현 == 4096
#define CHUNKSIZE (1<<12)
// X Y 중 큰 값 표출, 같으면 y 표출
#define MAX(x, y) ((x)>(y) ? (x):(y))
// alloc 비트를 사이즈에 주입(OR 연산), size는 0비트로 되어있어야 할 것 같음
#define PACK(size, alloc) ((size)|(alloc))

//주소에서 읽고 쓰기 함수
//변수 P를 양수 INT로 형 변환 하여 포인터가 가진 값(주소정보)로 리턴
#define GET(p) (*(unsigned int*) (p))
//변수 P를 양수 INT로 형 변환 하고 값을 입력
#define PUT(p, val) (*(unsigned int*) (p) = (val))

//헤더,풋터 정보 GET 함수
//사이즈 정보는 블록의 끝의 3자리를 제외한 나머지로 판단
#define GET_SIZE(p) (GET(p) & ~0x7)
//할당 여부 정보는 블록의 끝의 1자리로 판단
#define GET_ALLOC(p) (GET(p) & 0x1)

//헤더와 풋터 위치 체크
//블록포인터에 1워드 뒤로 이동(char 1 byte, WSIZE 4 byte) 4칸 뒤로이동한 포인터 반환
#define HDRP(bp) ((char*) (bp) - WSIZE)
//블록포인터에 사이즈 만큼 이동하고 footer 이동한 포인터 반환
#define FTRP(bp) ((char*) (bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 다음 블록 포인터로 찾기
#define NEXT_BLKP(bp) ((char*) (bp) + GET_SIZE(((char*) (bp) - WSIZE)))
// 이전 블록 포인터로 찾기
#define PREV_BLKP(bp) ((char*) (bp) - GET_SIZE(((char*) (bp) - DSIZE)))

/* single word (4) or double word (8) alignment  워드 : 4, 더블 워드 : 8 */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
/* ALIGNMENT의 가장 가까운 배수로 반올림 */
/* 원리 : 그냥 3 비트 내릴 경우 현재 사이즈보다 작아질 수 있기 때문에 8보다 1적은 7을 더함 
  8일 경우는 더커짐으로 7 대입 */
/* 뒤에 세비트가 111 인 경우 7을 의미함, 그런다면 해당 비트를 000으로 하면 7이하 내림이 가능 */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

/* 아키텍처의 메모리 할당 사이즈를 ALIGNMENT(8)로 정렬 하기 위한 매크로 */
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// explicit 용 매크로
#define PRED_FREEP(bp) (*(void**)(bp))
#define SUCC_FREEP(bp) (*(void**)(bp + WSIZE))

// 사용 힙 포인터 생성
static char *heap_listp;
// 마지막 BP
static char *last_bp;
// LIFO 용 헤더 BP
static char *LIFO;

void putFreeBlock(void *bp){
    SUCC_FREEP(bp) = heap_listp;
    PRED_FREEP(bp) = NULL;
    PRED_FREEP(heap_listp) = bp;
    heap_listp = bp;
}
// free list 맨 앞에 프롤로그 블록이 존재
void removeBlock(void *bp){
    // 첫 번째 블록을 없앨 때
    if(bp == heap_listp){
        PRED_FREEP(SUCC_FREEP(bp)) = NULL;
        heap_listp = SUCC_FREEP(bp);
    }else{
        SUCC_FREEP(PRED_FREEP(bp)) = SUCC_FREEP(bp);
        PRED_FREEP(SUCC_FREEP(bp)) = PRED_FREEP(bp);
    }
}

//find_fit
static void *find_fit(size_t asize){
  void *bp;
  // @@@@ explicit에서 추가 @@@@
  // 가용리스트 내부의 유일한 할당블록인 프롤로그 블록을 만나면 종료
  for(bp = heap_listp; GET_ALLOC(HDRP(bp)) != 1; bp = SUCC_FREEP(bp)){
      if(GET_SIZE(HDRP(bp)) >= asize){
          return bp;
      }
  }
  return NULL; // No fit
}

static void *next_fit(size_t adjusted_size) {
    char *bp = last_bp;

    for (bp = NEXT_BLKP(bp); GET_SIZE(HDRP(bp)) != 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= adjusted_size) {
            last_bp = bp;
            return bp;
        }
    }

    bp = heap_listp;
    while (bp < last_bp) {
        bp = NEXT_BLKP(bp);
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= adjusted_size) {
            last_bp = bp;
            return bp;
        }
    }

    return NULL;
}

static void place(void *bp, size_t asize){
  size_t csize = GET_SIZE(HDRP(bp));
  removeBlock(bp);
  if ((csize - asize) >= (2*DSIZE)){
      PUT(HDRP(bp), PACK(asize,1));//현재 크기를 헤더에 집어넣고
      PUT(FTRP(bp), PACK(asize,1));
      bp = NEXT_BLKP(bp);
      PUT(HDRP(bp), PACK(csize-asize,0));
      PUT(FTRP(bp), PACK(csize-asize,0));
      putFreeBlock(bp); // free list 첫번째에 분할된 블럭을 넣는다.
  }
  else{
      PUT(HDRP(bp), PACK(csize,1));
      PUT(FTRP(bp), PACK(csize,1));
  }
}
//연결
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); //이전 블록이 할당되었는지 아닌지 0 or 1
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); //다음 블록이 할당되었는지 아닌지 0 or 1
    size_t size = GET_SIZE(HDRP(bp)); //현재 블록 사이즈 
    // @@@@ explicit에서 추가 @@@@
    // case 1 - 가용블록이 없으면 조건을 추가할 필요 없다. 맨 밑에서 freelist에 넣어줌
    // case 2
    if(prev_alloc && !next_alloc){
        removeBlock(NEXT_BLKP(bp)); // @@@@ explicit에서 추가 @@@@
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));//header가 바뀌었으니까 size도 바뀐다!
    }
    // case 3
    else if(!prev_alloc && next_alloc){
        removeBlock(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));
        // PUT(FTRP(bp), PACK(size,0));  // @@@@ explicit에서 추가 @@@@ - 여기 다르긴함
        // PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        // bp = PREV_BLKP(bp); //bp를 prev로 옮겨줌
    }
    // case 4
    else if(!prev_alloc && !next_alloc){
        removeBlock(PREV_BLKP(bp));
        removeBlock(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp); //bp를 prev로 옮겨줌
    }
    putFreeBlock(bp); // 연결이 된 블록을 free list 에 추가
    return bp;
}

//힙데이터 입력 수행
static void *extend_heap(size_t words){
  //블록 포인터 생성
  char *bp;
  //사이즈 측정 변수 생성
  size_t size;

  //사이즈 측정, 현재 더블워드로 관리되기 때문에 홀수 byte의 경우 1의 패딩 추가
  //워드 사이즈 곱하여 사이즈로 지정
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  //현재 사이즈만큼 확장이 불가한 경우 실패 처리
  if((long)(bp = mem_sbrk(size)) == -1)
    return NULL;
  //헤더셋팅
  PUT(HDRP(bp), PACK(size, 0));
  //풋터셋팅
  PUT(FTRP(bp), PACK(size, 0));
  //에필로그 셋팅
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
  //병합 작업
  return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package. 말록 패키지 초기화
 */
int mm_init(void)
{
  
  printf("%p\n", SIZE_T_SIZE);
  printf("%p\n", sizeof(size_t));
  //힙 메모리 16 바이트 증가가 성공했는지 체크 실패시 리턴 -1
  if((heap_listp = mem_sbrk(6*WSIZE)) == (void*)-1)
    return -1;
  //0 포인터 값을 0으로 초기화 : 시작 패딩
  PUT(heap_listp, 0);
  //포인터 값에 1워드 전진 후 사이즈 1로 설정 : 프롤로그 1
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE*2 , 1));
  //next 2워드
  PUT(heap_listp + (2*WSIZE), NULL);
  //prev 3워드
  PUT(heap_listp + (3*WSIZE), NULL);
  //포인터 값에 4워드 전진 후 사이즈 1로 설정 : 프롤로그 2
  PUT(heap_listp + (4*WSIZE), PACK(DSIZE*2 , 1));
  //포인터 값에 5워드 전진 후 사이즈 1로 설정 : 에필로그
  PUT(heap_listp + (5*WSIZE), PACK(0 , 1));
  //포인터 값에 2워드 전진 값을 현재 포인터로 설정 프롤로그 1 뒤
  heap_listp += (2*WSIZE);
  //청크 사이즈에 헤더, 풋터, next, prev 를 나눈 갯수만큼 확장한 결과가 NULL이면 오류
  if(extend_heap(CHUNKSIZE/DSIZE) == NULL)
    return -1;
  //정상 처리
  return 0;
}

/* 
  mm_malloc - Allocate a block by incrementing the brk pointer.
  mm_malloc - brk 포인터를 증가시켜 블록을 할당합니다.
  Always allocate a block whose size is a multiple of the alignment.
  항상 크기가 정렬의 배수인 블록을 할당합니다.
*/

void *mm_malloc(size_t size)
{
  //adjusted 조정된 블록 사이즈
  size_t asize;
  //확장량 사이즈
  size_t extendsize;
  //블록 포인터 지정
  char *bp;

  if(size == 0)
    return NULL;
  
  if(size <= DSIZE)
    asize = 2 * DSIZE;
  else
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
  if((bp = find_fit(asize)) != NULL){
    place(bp, asize);
    return bp;
  }

  extendsize = MAX(asize, CHUNKSIZE);
  if((bp = extend_heap(extendsize/WSIZE)) == NULL)
    return NULL;
  place(bp, asize);
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
   mm_free - 블록 해제는 아무 작업도 수행하지 않습니다.
 */
void mm_free(void *bp)
{
  size_t size = GET_SIZE(HDRP(bp));

  PUT(HDRP(bp), PACK(size,0));
  PUT(FTRP(bp), PACK(size,0));
  coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
   mm_realloc - 단순히 mm_malloc과 mm_free를 사용하여 구현됩니다.
 */

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    //copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE; 
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

// void *mm_realloc(void *ptr, size_t size)
// {
//     size_t oldSize = GET_SIZE(HDRP(ptr)) - DSIZE;
//     void* newptr;
//     if (size <= oldSize){
//       return ptr;
//     }
//     else{
//       newptr = mm_malloc(oldSize + ((size-oldSize)));
//       if (newptr == NULL)
//         return NULL;
//       memcpy(newptr, ptr, oldSize);
//       mm_free(ptr);
//       return newptr;
//     }
// }