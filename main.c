#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct number {
    uint8_t len;
    uint8_t *data;
};

struct number New_number(uint8_t len){
    struct number New_n;
    New_n.len = len;
    New_n.data = calloc(len, 1);
    return New_n;
}

void del_number(struct number *n){
    free((*n).data);
}

struct number Copy_number(struct number *n){
    struct number New_n = New_number((*n).len);
    memcpy(New_n.data, (*n).data, (*n).len);
    return New_n;
}

void extend_number(struct number *n, uint8_t new_len){
    uint8_t old_len = (*n).len;
    uint8_t *old_data = (*n).data;
    (*n).len = new_len;
    (*n).data = calloc(new_len, 1);
    memcpy((*n).data, old_data, old_len);
}

uint8_t get_bit(struct number *n, uint16_t bit){
    if(bit >= (*n).len * 8){
        return 0;
    }
    uint8_t mask = 0b00000001 << (bit % 8);
    if((*n).data[bit / 8] & mask){
        return 1;
    }else{
        return 0;
    }
}

void set_bit(struct number *n, uint16_t bit, uint8_t val){
    if(val){
        if(bit >= (*n).len * 8){
            extend_number(n, (*n).len + 1);
        }
        uint8_t mask = 0b00000001 << (bit % 8);
        (*n).data[bit / 8] = (*n).data[bit / 8] | mask;
    }
}

void clear(struct number *n){
    for(uint8_t byte_n = 0; byte_n < (*n).len; byte_n++){
        (*n).data[byte_n] = 0;
    }
}

struct number Add_numbers(struct number *a, struct number *b){
    uint8_t len;
    if((*a).len >= (*b).len){
        len = (*a).len;
    }else{
        len = (*b).len;
    }
    struct number Ans = New_number(len);

    uint8_t carry = 0;
    uint8_t a_bit;
    uint8_t b_bit;
    uint16_t bit_len = ((uint16_t)len) * 8;
    for(uint16_t bin_n = 0; bin_n < bit_len || carry; bin_n++){
        a_bit = get_bit(a, bin_n);
        b_bit = get_bit(b, bin_n);
        set_bit(&Ans, bin_n, a_bit ^ b_bit ^ carry);
        carry = (a_bit & b_bit) | (a_bit & carry) | (b_bit & carry);
    }
    return Ans;
}

void bit_shift(struct number *n, uint16_t shift){
    struct number Temp = Copy_number(n);
    clear(n);
    uint16_t bit_len = ((uint16_t)(*n).len) * 8;
    for(uint16_t bit_n = 0; bit_n < bit_len; bit_n++){
        set_bit(n, bit_n + shift, get_bit(&Temp, bit_n));
    }
    del_number(&Temp);
}

struct number Multiply_numbers(struct number *a, struct number *b){
    struct number Shifted_b = Copy_number(b);
    struct number Ans = New_number((*b).len);
    struct number Temp;
    uint16_t a_bit_len = ((uint16_t)(*a).len) * 8;

    for(uint16_t shift = 0; shift < a_bit_len; shift++){
        if(get_bit(a, shift)){
            Temp = Add_numbers(&Ans, &Shifted_b);
            del_number(&Ans);
            Ans = Temp;
        }
        bit_shift(&Shifted_b, 1);
    }
    del_number(&Shifted_b);
    return Ans;
}

int main(){
    struct number A = New_number(1);
    struct number B = New_number(1);

    A.data[0] = 0xff;
    B.data[0] = 0xff;

    struct number Ans = Multiply_numbers(&A, &B);
    printf("%X %X\n", Ans.data[1], Ans.data[0]);

    return 0;
}
