#include "obj.h"
#include "result.h"
#include "stdio.h"
#include "vec.h"
typedef struct State State;
typedef enum Key Key;

struct State {
  // CVec<CResult<i32>>
  Vec num;
};
static void state_drop(void *state_ptr) {
  State *state = state_ptr;
  vec_drop(&state->num);
}
static void *state_clone_obj(const void *source_ptr) {
  const State *source = source_ptr;
  ResultVec result = vec_clone(&source->num);
  State *copy;

  if (Vec_is_err(&result))
    return NULL;

  copy = malloc(sizeof(*copy));
  if (copy == NULL) {
    ResultVec_drop(&result);
    return NULL;
  }
  copy->num = Vec_unwrap(&result);
  return copy;
}

IMPL_SHY_OWNED_RESULT(State, state_drop, state_clone_obj)

enum Key {
  Up = 1,
  Down = 2,
  Left = 3,
  Right = 4,
};
/*函数声明*/
void update(State *state, Key key);
ResultState init(void);
Resulti32 *index(State *state, usize i, usize j);

void draw(State *state) {
  printf("\033[2J\033[H");
  printf("+------+------+------+------+\n");
  for (usize j = 0; j < 4; j++) {
    for (usize i = 0; i < 4; i++) {
      const Resulti32 *cell = index(state, i, j);
      if (i32_is_ok(cell))
        printf("|%6d", *i32_unwrap_ref(cell));
      else
        printf("|      ");
    }
    printf("|\n");
    printf("+------+------+------+------+\n");
  }
}

Key get_key(){
    while(1){
    var c=getchar();
    switch(c){
        case 'w':
            return Up;
        case 's':
            return Down;
        case 'a':
            return Left;
        case 'd':
            return Right;
        }
    }
}
int main(void) {
    let(ResultState) s=init();
    let(State) state=State_unwrap(&s);
    draw(&state);
    var exit=0;
    while(!exit){
        var key=get_key();
        update(&state,key);
        draw(&state);
    }
    return 0;
}
