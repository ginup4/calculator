#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef uint8_t bool;
struct number_struct {
    uint16_t len;
    uint8_t *data;
};
typedef struct number_struct* number;

#define LINE_SIZE 256
#define LINE_N 256

FILE *input_file = NULL;
FILE *output_file = NULL;
char lines[LINE_N][LINE_SIZE];
int lines_count = 0;

void PRINT_NUMBER(number n){
    for(int i = (*n).len - 1; i >= 0; i--){
        printf("%x ", (*n).data[i]);
    }
    printf("\n");
}

void crash(const char *msg){
    fprintf(stderr, "%s", msg);
    exit(1);
}

void warning(const char *msg){
    fprintf(stderr, "%s", msg);
}

number new_number(){
    number new_n = (number)calloc(1, sizeof(struct number_struct));
    (*new_n).len = 1;
    (*new_n).data = calloc(1, 1);
    return new_n;
}

void del_number(number n){
    free((*n).data);
    free(n);
}

void set_number(number n, number val){
    (*n).len = (*val).len;
    free((*n).data);
    (*n).data = calloc((*val).len, 1);
    memcpy((*n).data, (*val).data, (*val).len);
}

void clear_number(number n){
    free((*n).data);
    (*n).len = 1;
    (*n).data = calloc(1, 1);
}

void set_number_from_uint8_t(number n, uint8_t val){
    clear_number(n);
    (*n).data[0] = val;
}

void extend_number(number n, uint16_t new_len){
    uint16_t old_len = (*n).len;
    uint8_t *old_data = (*n).data;
    (*n).len = new_len;
    (*n).data = calloc(new_len, 1);
    memcpy((*n).data, old_data, old_len);
}

bool get_bit(number n, int32_t bit){
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

uint16_t get_len(number n){
    return (*n).len;
}

void set_bit(number n, int32_t bit, bool val){
    if(val){
        if(bit >= ((int32_t)(*n).len) * 8){
            extend_number(n, bit / 8 + 1);
        }
        uint8_t mask = 0b00000001 << (bit % 8);
        (*n).data[bit / 8] = (*n).data[bit / 8] | mask;
    }
}

void add(number a, number b, number ret){
    number ans = new_number();

    uint16_t len;
    if(get_len(a) > get_len(b)){
        len = get_len(a);
    }else{
        len = get_len(b);
    }
    bool carry = 0;
    bool a_bit;
    bool b_bit;
    int32_t bit_len = ((int32_t)len) * 8;
    for(int32_t bit_n = 0; bit_n < bit_len || carry; bit_n++){
        a_bit = get_bit(a, bit_n);
        b_bit = get_bit(b, bit_n);
        set_bit(ans, bit_n, a_bit ^ b_bit ^ carry);
        carry = (a_bit & b_bit) | (a_bit & carry) | (b_bit & carry);
    }

    set_number(ret, ans);
    del_number(ans);
}

bool compare(number a, number b){
    int32_t bit_len;
    if(get_len(a) >= get_len(b)){
        bit_len = ((int32_t) get_len(a)) * 8;
    }else{
        bit_len = ((int32_t) get_len(b)) * 8;
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

void subtract(number a, number b, number ret){
    number ans = new_number();

    uint16_t len;
    if(get_len(a) > get_len(b)){
        len = get_len(a);
    }else{
        len = get_len(b);
    }
    bool carry = 0;
    bool a_bit;
    bool b_bit;
    int32_t bit_len = ((int32_t)len) * 8;
    for(int32_t bit_n = 0; bit_n < bit_len || carry; bit_n++){
        a_bit = get_bit(a, bit_n);
        b_bit = get_bit(b, bit_n);
        set_bit(ans, bit_n, a_bit ^ b_bit ^ carry);
        carry = a_bit < b_bit + carry;
    }

    set_number(ret, ans);
    del_number(ans);
}

void bit_shift_number(number n, int32_t shift, number ret){
    number ans = new_number();

    int32_t bit_len = ((int32_t) get_len(n)) * 8;
    for(int32_t bit_n = 0; bit_n < bit_len; bit_n++){
        if(bit_n + shift >= 0){
            set_bit(ans, bit_n + shift, get_bit(n, bit_n));
        }
    }

    set_number(ret, ans);
    del_number(ans);
}

void multiply(number a, number b, number ret){
    number shifted_b = new_number();
    number ans = new_number();

    set_number(shifted_b, b);
    int32_t a_bit_len = ((int32_t) get_len(a)) * 8;
    for(int32_t shift = 0; shift < a_bit_len; shift++){
        if(get_bit(a, shift)){
            add(ans, shifted_b, ans);
        }
        bit_shift_number(shifted_b, 1, shifted_b);
    }

    set_number(ret, ans);
    del_number(shifted_b);
    del_number(ans);
}

void divide(number a, number b, number ret_quotient, number ret_rest){
    number ans = new_number();
    number rest = new_number();
    number shifted_b = new_number();

    int32_t a_bit_len = ((int32_t) get_len(a)) * 8;
    set_number(rest, a);
    bit_shift_number(b, a_bit_len, shifted_b);

    bool can_subtract;
    for(int32_t bit_n = a_bit_len - 1; bit_n >= 0; bit_n--){
        bit_shift_number(shifted_b, -1, shifted_b);
        can_subtract = compare(rest, shifted_b);
        set_bit(ans, bit_n, can_subtract);
        if(can_subtract){
            subtract(rest, shifted_b, rest);
        }
    }

    if(ret_quotient){
        set_number(ret_quotient, ans);
    }
    if(ret_rest){
        set_number(ret_rest, rest);
    }
    del_number(ans);
    del_number(rest);
    del_number(shifted_b);
}

void close_files(){
    if(input_file){
        fclose(input_file);
    }
    if(output_file){
        fclose(output_file);
    }
}

bool is_operation(const char *str){
    int i;
    bool got_space = 0;
    if(str[0] == '+' || str[0] == '*' || str[0] == '/' || str[0] == '%' || str[0] == '^'){
        if(str[1] != ' '){
            return 0;
        }else{
            i = 2;
            got_space = 1;
        }
    }else{
        i = 0;
    }
    for(; str[i] != '\0'; i++){
        if(str[i] == ' '){
            if(got_space){
                return 0;
            }else{
                got_space = 1;
            }
        }else if(!isdigit(str[i])){
            return 0;
        }
    }
    if(!got_space){
        return 0;
    }
    return 1;
}

bool is_number(const char *str){
    for(int i = 0; str[i] != '\0'; i++){
        if(!isalnum(str[i])){
            return 0;
        }
    }
    return 1;
}

void execute_calculation(){
    if(lines_count < 2){
        return;
    }
    if(!is_operation(lines[0])){
        return;
    }
    for(int i = 1; i < lines_count; i++){
        if(!is_number(lines[i])){
            return;
        }
    }

    printf("lines: %i\n", lines_count);
    for(int i = 0; i < lines_count; i++){
        printf("%s\n", lines[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]){
    uint64_t inp_path_len, out_path_len;

    if(argc == 1){
        fprintf(stderr, "input file not provided\n");
        return 1;
    }else if(argc == 2){
        inp_path_len = strlen(argv[1]);
        out_path_len = inp_path_len + 4;
    }else{
        inp_path_len = strlen(argv[1]);
        out_path_len = strlen(argv[2]);
    }
    char inp_path[inp_path_len + 1];
    char out_path[out_path_len + 1];
    strcpy(inp_path, argv[1]);
    if(argc == 2){
        strcpy(out_path, "out_");
        strcat(out_path, argv[1]);
    }else{
        strcpy(out_path, argv[2]);
    }


    input_file = fopen(inp_path, "r");
    if(!input_file){
        fprintf(stderr, "couldn't read %s\n", inp_path);
        close_files();
        return 1;
    }
    output_file = fopen(out_path, "w");
    if(!output_file){
        fprintf(stderr, "couldn't write to %s\n", out_path);
        close_files();
        return 1;
    }

    char buffer[LINE_SIZE];
    int buffer_counter = 0;

    bool reading_line = 0;
    int new_lines_n = 0;

    int raw_c;
    char c;
    while((raw_c = getc(input_file)) != EOF){
        c = (char) raw_c;

        if(c != '\n'){
            new_lines_n = 0;
            reading_line = 1;
            buffer[buffer_counter] = c;
            buffer_counter++;
            if(buffer_counter >= LINE_SIZE - 1){
                //line too long
            }
        }else{
            new_lines_n++;
            if(new_lines_n == 4){
                execute_calculation();
                lines_count = 0;
            }

            if(reading_line){
                buffer[buffer_counter] = '\0';
                strcpy(lines[lines_count], buffer);
                lines_count++;
                if(lines_count >= LINE_N){
                    //too many lines
                }
                buffer_counter = 0;
                reading_line = 0;
            }
        }
    }
    if(lines_count > 0){
        execute_calculation();
    }

    close_files();
    return 0;
}
