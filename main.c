#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct number {
    uint8_t len;
    uint8_t *data;
};

struct number new_number(uint8_t len){
    struct number n;
    n.len = len;
    n.data = calloc(len, 1);
    return n;
}

void del_number(struct number n){
    free(n.data);
}

struct number copy_number(struct number n, uint8_t new_len){
    struct number new_n = new_number(new_len);
    memcpy(new_n.data, n.data, n.len);
    return new_n;
}

struct number add_numbers(struct number a, struct number b){
    uint8_t len;
    if(a.len > b.len){
        if(a.data[a.len - 1] & 0b10000000){
            len = a.len + 1;
        }else{
            len = a.len;
        }
    }else if(b.len > a.len){
        if(b.data[b.len - 1] & 0b10000000){
            len = b.len + 1;
        }else{
            len = b.len;
        }
    }else{
        if((b.data[b.len - 1] & 0b10000000) || (a.data[a.len - 1] & 0b10000000)){
            len = a.len + 1;
        }else{
            len = a.len;
        }
    }
    struct number ans = new_number(len);
    a = copy_number(a, len);
    b = copy_number(b, len);

    uint8_t carry = 0b00000000;
    uint8_t mask;
    for(int byte = 0; byte < len; byte++){
        mask = 0b00000001;
        for(int i = 0; i < 8; i++){
            ans.data[byte] = ans.data[byte] | (mask & (a.data[byte] ^ b.data[byte] ^ carry));
            carry = mask & (a.data[byte] & b.data[byte]) | (a.data[byte] & carry) | (b.data[byte] & carry);

            mask = mask << 1;
            if(carry == 0b10000000){
                carry = 0b00000001;
            }else{
                carry = carry << 1;
            }
        }
    }
    del_number(a);
    del_number(b);
    return ans;
}

int main(){
    struct number a = new_number(8);
    struct number b = new_number(3);
    a.data[0] = 0xff;
    b.data[0] = 0xff;
    struct number ans = add_numbers(a, b);

    printf("%X %X %X %X %X %X %X %X", ans.data[7], ans.data[6], ans.data[5], ans.data[4], ans.data[3], ans.data[2], ans.data[1], ans.data[0]);
    return 0;
}
