#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h> // only to calculate how many digits a number has

//type definitions
typedef uint8_t bool;
typedef uint32_t chunk;
typedef uint64_t dblchunk;
typedef int32_t signed_chunk;
typedef int64_t signed_dblchunk;
typedef uint32_t len_t;
typedef int64_t bit_len_t;
const int chunk_size = sizeof(chunk);
const int chunk_bit_size = sizeof(chunk) * 8;
struct number_struct {
    len_t len;
    chunk *data;
};
typedef struct number_struct* number;

//consts and global variables definitions

#define LINE_SIZE 42

FILE *input_file = NULL;
FILE *output_file = NULL;

char operation = 'x';
chunk inp_base = 2;
chunk out_base = 2;
number num1, num2;
int numbers_n;
char buffer[LINE_SIZE];
int buffer_counter = 0;

bool reading_line = 0;
bool skip_line = 0;
int newlines_n = 0;


//number "object" "methods"

//"constructor"
number new_number(len_t len){
    number new_n = (number)calloc(1, sizeof(struct number_struct));
    new_n->len = len;
    new_n->data = calloc(new_n->len, chunk_size);
    return new_n;
}

len_t bits_to_len(bit_len_t bit_len){
    return bit_len / chunk_bit_size + (bit_len % chunk_bit_size > 0);
}

//"destructor"
void del_number(number n){
    free(n->data);
    free(n);
}

void set_number(number n, number val){
    len_t len = val->len;
    for(; len > 0; len--){
        if(val->data[len - 1]){
            break;
        }
    }
    n->len = len;
    free(n->data);
    n->data = calloc(len, chunk_size);
    memcpy(n->data, val->data, len * chunk_size);
}

void clear_number(number n){
    free(n->data);
    n->len = 1;
    n->data = calloc(1, chunk_size);
}

void set_number_from_chunk(number n, chunk val){
    clear_number(n);
    n->data[0] = val;
}

void extend_number(number n, len_t new_len){
    len_t old_len = n->len;
    chunk *old_data = n->data;
    n->len = new_len;
    n->data = calloc(new_len, chunk_size);
    memcpy(n->data, old_data, old_len * chunk_size);
}

bool get_bit(number n, bit_len_t bit){
    if(bit >= ((bit_len_t) n->len) * chunk_bit_size){
        return 0;
    }
    chunk mask = 1 << (bit % chunk_bit_size);
    if(n->data[bit / chunk_bit_size] & mask){
        return 1;
    }else{
        return 0;
    }
}

bit_len_t get_bit_len(number n){
    bit_len_t bit_n = ((bit_len_t) n->len) * chunk_bit_size;
    for(; bit_n > 0; bit_n--){
        if(get_bit(n, bit_n - 1)){
            return bit_n;
        }
    }
    return 0;
}

void set_bit(number n, bit_len_t bit, bool val){
    if(val){
        if(bit >= ((bit_len_t) n->len) * chunk_bit_size){
            extend_number(n, bit / chunk_bit_size + 1);
        }
        chunk mask = 1 << (bit % chunk_bit_size);
        n->data[bit / chunk_bit_size] = n->data[bit / chunk_bit_size] | mask;
    }
}


//arithmetic and logic operations on number "objects"

void add(number a, number b, number ret, len_t chunk_shift){
    len_t n1_shift = 0;
    len_t n2_shift = 0;
    len_t len;
    len_t small_len;
    number n1, n2;
    if(a->len >= b->len + chunk_shift){
        small_len = b->len + chunk_shift;
        len = a->len;
        n1 = a;
        n2 = b;
        n2_shift = chunk_shift;
    }else{
        small_len = a->len;
        len = b->len + chunk_shift;
        n1 = b;
        n2 = a;
        n1_shift = chunk_shift;
    }
    number ans = new_number(len);

    dblchunk n1_dblchunk;
    dblchunk n2_dblchunk;
    chunk carry = 0;
    len_t i = 0;
    for(; i < small_len; i++){
        if(i < n1_shift){
            n1_dblchunk = 0;
        }else{
            n1_dblchunk = n1->data[i - n1_shift];
        }
        if(i < n2_shift){
            n2_dblchunk = 0;
        }else{
            n2_dblchunk = n2->data[i - n2_shift];
        }
        n1_dblchunk = n1_dblchunk + n2_dblchunk + carry;
        carry = n1_dblchunk >> chunk_bit_size;
        ans->data[i] = (chunk) n1_dblchunk;
    }
    for(; i < len; i++){
        if(i < n1_shift){
            n1_dblchunk = 0;
        }else{
            n1_dblchunk = n1->data[i - n1_shift];
        }
        n1_dblchunk = n1_dblchunk + carry;
        carry = n1_dblchunk >> chunk_bit_size;
        ans->data[i] = (chunk) n1_dblchunk;
    }
    if(carry){
        extend_number(ans, len + 1);
        ans->data[len] = carry;
    }

    set_number(ret, ans);
    del_number(ans);
}

bool compare(number a, number b){
    bit_len_t a_bit_len = get_bit_len(a);
    bit_len_t b_bit_len = get_bit_len(b);
    bit_len_t bit_len = a_bit_len > b_bit_len ? a_bit_len : b_bit_len;

    for(bit_len_t bit_n = bit_len - 1; bit_n >= 0; bit_n--){
        if(get_bit(a, bit_n) > get_bit(b, bit_n)){
            return 1;
        }else if(get_bit(a, bit_n) < get_bit(b, bit_n)){
            return 0;
        }
    }
    return 1;
}

void subtract(number a, number b, number ret){
    len_t a_len = a->len;
    len_t b_len = b->len;
    number ans = new_number(a->len);

    signed_dblchunk a_dblchunk;
    signed_dblchunk b_dblchunk;
    signed_chunk carry = 0;

    len_t i = 0;
    for(; i < b_len; i++){
        a_dblchunk = a->data[i];
        b_dblchunk = b->data[i];
        a_dblchunk = a_dblchunk - b_dblchunk + carry;
        ans->data[i] = (chunk) * (dblchunk*) &a_dblchunk;
        carry = (signed_chunk) (a_dblchunk >> chunk_bit_size);
    }
    for(; i < a_len; i++){
        a_dblchunk = a->data[i];
        a_dblchunk = a_dblchunk + carry;
        ans->data[i] = (chunk) * (dblchunk*) &a_dblchunk;
        carry = (signed_chunk) (a_dblchunk >> chunk_bit_size);
    }

    set_number(ret, ans);
    del_number(ans);
}

void bit_shift(number n, bit_len_t shift, number ret){
    bit_len_t bit_len = get_bit_len(n);
    number ans = new_number(bits_to_len(bit_len + shift));

    for(bit_len_t bit_n = 0; bit_n < bit_len; bit_n++){
        if(bit_n + shift >= 0){
            set_bit(ans, bit_n + shift, get_bit(n, bit_n));
        }
    }

    set_number(ret, ans);
    del_number(ans);
}

void multiply(number a, number b, number ret){
    number ans = new_number(1);
    number partial_ans = new_number(b->len);
    number partial_carry = new_number(b->len);

    dblchunk a_dblchunk;
    dblchunk b_dblchunk;
    for(len_t a_chunk_n = 0; a_chunk_n < a->len; a_chunk_n++){
        a_dblchunk = a->data[a_chunk_n];
        for(len_t b_chunk_n = 0; b_chunk_n < b->len; b_chunk_n++){
            b_dblchunk = b->data[b_chunk_n];
            b_dblchunk = a_dblchunk * b_dblchunk;
            partial_ans->data[b_chunk_n] = (chunk) b_dblchunk;
            partial_carry->data[b_chunk_n] = b_dblchunk >> chunk_bit_size;
        }
        add(ans, partial_ans, ans, a_chunk_n);
        add(ans, partial_carry, ans, a_chunk_n + 1);
    }

    del_number(partial_ans);
    del_number(partial_carry);
    set_number(ret, ans);
    del_number(ans);
}

void divide(number a, number b, number ret_quotient, number ret_rest){
    number quotient = new_number(1);
    number rest = new_number(1);

    for(bit_len_t bit_n = get_bit_len(a) - 1; bit_n >= 0; bit_n--){
        bit_shift(rest, 1, rest);
        set_bit(rest, 0, get_bit(a, bit_n));
        if(compare(rest, b)){
            subtract(rest, b, rest);
            set_bit(quotient, bit_n, 1);
        }
    }

    if(ret_quotient){
        set_number(ret_quotient, quotient);
    }
    if(ret_rest){
        set_number(ret_rest, rest);
    }
    del_number(quotient);
    del_number(rest);
}

void long_divide(number a, chunk b, number ret_quotient, chunk *ret_rest){
    len_t len = a->len;
    number quotient = new_number(len);
    dblchunk a_dblchunk = 0;

    for(len_t i = 0; i < len; i++){
        a_dblchunk = a_dblchunk << chunk_bit_size;
        a_dblchunk = a_dblchunk + a->data[len - i - 1];
        quotient->data[len - i - 1] = a_dblchunk / b;
        a_dblchunk = a_dblchunk % b;
    }

    *ret_rest = (chunk) a_dblchunk;
    set_number(ret_quotient, quotient);
    del_number(quotient);
}

void exponentiate(number a, number b, number ret){
    bit_len_t b_bit_len = get_bit_len(b);

    number ans = new_number(1);
    number multiplied_a = new_number(1);
    set_number_from_chunk(ans, 1);
    set_number(multiplied_a, a);

    for(bit_len_t bit_n = 0; bit_n < b_bit_len; bit_n++){
        if(get_bit(b, bit_n)){
            multiply(ans, multiplied_a, ans);
        }
        multiply(multiplied_a, multiplied_a, multiplied_a);
    }

    set_number(ret, ans);
    del_number(ans);
    del_number(multiplied_a);
}

void close_files(){
    if(input_file){
        fclose(input_file);
    }
    if(output_file){
        fclose(output_file);
    }
}


//functions used for parsing the input file and printing to the output file

chunk val_of_digit(char c){
    if('0' <= c && c <= '9'){
        return c - '0';
    }else if('a' <= c && c <= 'z'){
        return c - 'a' + 10;
    }else if('A' <= c && c <= 'Z'){
        return c - 'A' + 10;
    }else{
        return 0; // shouldn't ever happen
    }
}

char digit_of_val(chunk val){
    if(val < 10){
        return '0' + val;
    }else if(val <= 16){
        return 'A' + val - 10;
    }else{
        return '!';
    }
}

bool is_operation(const char *str){
    int i;
    bool got_space = 0;
    int digit_count = 0;
    if(str[0] == '+' || str[0] == '*' || str[0] == '/' || str[0] == '%' || str[0] == '^'){
        if(str[1] != ' '){
            return 0;
        }else{
            i = 2;
            got_space = 1;
        }
    }else if(isdigit(str[0])){
        i = 0;
    }else{
        return 0;
    }
    for(; str[i] != '\0'; i++){
        if(str[i] == ' '){
            digit_count = 0;
            if(got_space){
                return 0;
            }else{
                got_space = 1;
            }
        }else if(!isdigit(str[i])){
            return 0;
        }else{
            digit_count++;
            if(digit_count > 2){
                return 0;
            }
        }
    }
    if(!got_space){
        return 0;
    }
    return 1;
}

bool is_number(const char *str, chunk base){
    for(int i = 0; str[i] != '\0'; i++){
        if(!isalnum(str[i])){
            return 0;
        }else{
            if(val_of_digit(str[i]) >= base){
                fprintf(stderr, "Digit value in number greater or equal to base.\n");
                return 0;
            }
        }
    }
    return 1;
}

void set_number_from_string(number n, const char *str, chunk base){
    number base_number = new_number(1);
    number weight = new_number(1);
    number digit_val = new_number(1);
    set_number_from_chunk(base_number, base);
    set_number_from_chunk(weight, 1);
    clear_number(n);

    int i = 0;
    while(str[i] != '\0'){
        i++;
    }
    i--;

    for(; i >= 0; i--){
        set_number_from_chunk(digit_val, val_of_digit(str[i]));
        multiply(digit_val, weight, digit_val);
        add(n, digit_val, n, 0);
        multiply(weight, base_number, weight);
    }

    del_number(base_number);
    del_number(weight);
    del_number(digit_val);
}

void print_number(FILE *stream, number n, chunk base){
    chunk digit_val;
    number zero = new_number(1);

    if(compare(zero, n)){
        fprintf(stream, "0");
    }else{
        int str_len = (int) (ceil((double) (*n).len * 32 / log2(base)) + 8);
        char *out_str = (char *) malloc(str_len);

        int i = 0;
        for(; !compare(zero, n); i++){
            long_divide(n, base, n, &digit_val);
            out_str[i] = digit_of_val(digit_val);
        }
        out_str[i] = '\0';
        strrev(out_str);

        fprintf(stream, "%s", out_str);
    }
    del_number(zero);
}

void process_line(){
    fprintf(output_file, "%s\n\n", buffer);
    if(is_operation(buffer)){
        if(isdigit(buffer[0])){
            operation = 'b';
            if(buffer[1] == ' '){
                if(buffer[3] == '\0'){
                    inp_base = val_of_digit(buffer[0]);
                    out_base = val_of_digit(buffer[2]);
                }else{
                    inp_base = val_of_digit(buffer[0]);
                    out_base = 10 * val_of_digit(buffer[2]) + val_of_digit(buffer[3]);
                }
            }else{
                if(buffer[4] == '\0'){
                    inp_base = 10 * val_of_digit(buffer[0]) + val_of_digit(buffer[1]);
                    out_base = val_of_digit(buffer[3]);
                }else{
                    inp_base = 10 * val_of_digit(buffer[0]) + val_of_digit(buffer[1]);
                    out_base = 10 * val_of_digit(buffer[3]) + val_of_digit(buffer[4]);
                }
            }
        }else{
            operation = buffer[0];
            if(buffer[3] == '\0'){
                inp_base = val_of_digit(buffer[2]);
            }else{
                inp_base = 10 * val_of_digit(buffer[2]) + val_of_digit(buffer[3]);
            }
            out_base = inp_base;
        }

        if(!(2 <= inp_base && inp_base <= 16)){
            fprintf(stderr, "The base has to be equal or less 16 (%u)\n", inp_base);
            return;
        }
        if(!(2 <= out_base && out_base <= 16)){
            fprintf(stderr, "The base has to be equal or less 16 (%u)\n", out_base);
            return;
        }
    }else if(operation != 'x'){
        if(is_number(buffer, inp_base)){
            if(numbers_n == 0){
                set_number_from_string(num1, buffer, inp_base);
            }else{
                set_number_from_string(num2, buffer, inp_base);
                switch(operation){
                    case '+':
                        add(num1, num2, num1, 0);
                        break;
                    case '*':
                        multiply(num1, num2, num1);
                        break;
                    case '/':
                        divide(num1, num2, num1, NULL);
                        break;
                    case '%':
                        divide(num1, num2, NULL, num1);
                        break;
                    case '^':
                        exponentiate(num1, num2, num1);
                        break;
                    default:
                        fprintf(stderr, "Only one number is allowed as input for base change.\n");
                        break;
                }
            }
            numbers_n++;
        }
    }else{
        fprintf(stderr, "Couldn't load number without operation and base set.\n");
    }
}

void print_answer(){
    if(numbers_n > 0){
        numbers_n = 0;
        print_number(output_file, num1, out_base);
        fprintf(output_file, "\n\n");
    }
}

void process_char(char c){
    if(c == '\n'){
        newlines_n++;
        if(newlines_n == 3){
            print_answer();
        }
        if(reading_line){
            buffer[buffer_counter] = '\0';
            process_line();
            buffer_counter = 0;
            reading_line = 0;
        }
    }else if(!skip_line && isprint(c)){
        newlines_n = 0;
        reading_line = 1;
        buffer[buffer_counter] = c;
        buffer_counter++;
        if(buffer_counter >= LINE_SIZE - 1){
            fprintf(stderr, "Line too long. (40 digits max)\n");
            skip_line = 1;
        }
    }
}

int main(int argc, char *argv[]){
    //checking the command line arguments and opening the input and output files
    uint64_t inp_path_len, out_path_len;

    if(argc == 1){
        fprintf(stderr, "Input file not provided.\n");
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
        fprintf(stderr, "Couldn't read %s\n", inp_path);
        close_files();
        return 1;
    }
    output_file = fopen(out_path, "w");
    if(!output_file){
        fprintf(stderr, "Couldn't write to %s\n", out_path);
        close_files();
        return 1;
    }

    //main parsing loop
    num1 = new_number(1);
    num2 = new_number(1);
    int raw_c;
    while((raw_c = getc(input_file)) != EOF){
        process_char((char) raw_c);
    }
    process_char('\n');
    print_answer();

    close_files();
    return 0;
}
