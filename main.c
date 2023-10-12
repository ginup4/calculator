#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct number {
    uint16_t len;
    uint8_t *data;
};

struct number New_number(uint16_t len){
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

void extend_number(struct number *n, uint16_t new_len){
    uint16_t old_len = (*n).len;
    uint8_t *old_data = (*n).data;
    (*n).len = new_len;
    (*n).data = calloc(new_len, 1);
    memcpy((*n).data, old_data, old_len);
}

uint8_t get_bit(struct number *n, int32_t bit){
    if(bit >= ((int32_t)(*n).len) * 8){
        return 0;
    }
    uint8_t mask = 0b00000001 << (bit % 8);
    if((*n).data[bit / 8] & mask){
        return 1;
    }else{
        return 0;
    }
}

void set_bit(struct number *n, int32_t bit, uint8_t val){
    if(val){
        if(bit >= ((int32_t)(*n).len) * 8){
            extend_number(n, bit / 8 + 1);
        }
        uint8_t mask = 0b00000001 << (bit % 8);
        (*n).data[bit / 8] = (*n).data[bit / 8] | mask;
    }
}

void clear_number(struct number *n){
    for(uint16_t byte_n = 0; byte_n < (*n).len; byte_n++){
        (*n).data[byte_n] = 0;
    }
}

struct number Add_numbers(struct number *a, struct number *b){
    uint16_t len;
    if((*a).len >= (*b).len){
        len = (*a).len;
    }else{
        len = (*b).len;
    }
    struct number Ans = New_number(len);

    uint8_t carry = 0;
    uint8_t a_bit;
    uint8_t b_bit;
    int32_t bit_len = ((int32_t)len) * 8;
    for(int32_t bin_n = 0; bin_n < bit_len || carry; bin_n++){
        a_bit = get_bit(a, bin_n);
        b_bit = get_bit(b, bin_n);
        set_bit(&Ans, bin_n, a_bit ^ b_bit ^ carry);
        carry = (a_bit & b_bit) | (a_bit & carry) | (b_bit & carry);
    }
    return Ans;
}

struct number Subtract_numbers(struct number *a, struct number *b){
    uint16_t len;
    if((*a).len >= (*b).len){
        len = (*a).len;
    }else{
        len = (*b).len;
    }
    struct number Ans = New_number(1);

    uint8_t carry = 0;
    uint8_t a_bit;
    uint8_t b_bit;
    int32_t bit_len = ((int32_t)len) * 8;
    for(int32_t bin_n = 0; bin_n < bit_len || carry; bin_n++){
        a_bit = get_bit(a, bin_n);
        b_bit = get_bit(b, bin_n);
        set_bit(&Ans, bin_n, a_bit ^ b_bit ^ carry);
        carry = a_bit < b_bit + carry;
    }
    return Ans;
}

void bit_shift(struct number *n, int32_t shift){
    struct number Temp = Copy_number(n);
    clear_number(n);
    int32_t bit_len = ((int32_t)(*n).len) * 8;
    for(int32_t bit_n = 0; bit_n < bit_len; bit_n++){
        if(bit_n + shift >= 0){
            set_bit(n, bit_n + shift, get_bit(&Temp, bit_n));
        }
    }
    del_number(&Temp);
}

struct number Multiply_numbers(struct number *a, struct number *b){
    struct number Shifted_b = Copy_number(b);
    struct number Ans = New_number((*b).len);
    struct number Temp;
    int32_t a_bit_len = ((int32_t)(*a).len) * 8;

    for(int32_t shift = 0; shift < a_bit_len; shift++){
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

uint8_t compare_numbers(struct number *a, struct number *b){
    int32_t bit_len;
    if((*a).len >= (*b).len){
        bit_len = ((int32_t)(*a).len) * 8;
    }else{
        bit_len = ((int32_t)(*b).len) * 8;
    }
    
    for(int32_t bit_n = bit_len - 1; bit_n >= 0; bit_n--){
        if(get_bit(a, bit_n) > get_bit(b, bit_n)){
            return 1;
        }else if(get_bit(a, bit_n) < get_bit(b, bit_n)){
            return 0;
        }
    }
    return 1;
}

struct number Divide_numbers(struct number *a, struct number *b, uint8_t if_rest){
    int32_t a_bit_len = ((int32_t)(*a).len) * 8;
    struct number Ans = New_number(1);
    struct number Rest = Copy_number(a);
    struct number Temp;
    bit_shift(b, a_bit_len);

    uint8_t is_bigger;
    for(int32_t bit_n = a_bit_len - 1; bit_n >= 0; bit_n--){
        bit_shift(b, -1);
        is_bigger = compare_numbers(&Rest, b);
        set_bit(&Ans, bit_n, is_bigger);
        if(is_bigger){
            Temp = Subtract_numbers(&Rest, b);
            del_number(&Rest);
            Rest = Temp;
        }
    }

    if(if_rest){
        del_number(&Ans);
        return Rest;
    }else{
        del_number(&Rest);
        return Ans;
    }
}

int main(){
    struct number A = New_number(1);
    struct number B = New_number(1);

    A.data[0] = 0x80;
    B.data[0] = 0x81;

    struct number Ans = Divide_numbers(&A, &B, 1);
    for(int i = Ans.len - 1; i >= 0; i--){
        printf("%x ", Ans.data[i]);
    }


    return 0;
}
