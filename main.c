#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>

#define EXIT_BUTTON 27
#define FILENAME_SIZE 11
#define MENU_LINES 12
#define REGION_NAME_MAX 21

#define RESET_TEXT "\x1b[0m"
#define BOLD_TEXT "\x1b[1m"
#define ITALIC_TEXT "\x1b[3m"

#define GREEN_TEXT "\x1b[92m"
#define BLACK_TEXT "\x1b[30m"

#define GREEN_BG "\x1b[102m"
#define BLACK_BG "\x1b[40m"

#define NOT_INTERACTIVE (-1)

const char *working_folder = "./files";

const int population_min = 0;
const int population_max = 1000000000;
const double area_min = 0;
const double area_max = 1e9;

enum action {
    CREATE_FILE = 1,
    OPEN_FILE,
    DELETE_FILE,
    CREATE_RECORD,
    READ_RECORD,
    DELETE_RECORD,
    EDIT_RECORD,
    ORDER_RECORDS,
    INSERT_RECORD,
};

enum sort_option {
    NAME_SORT,
    AREA_SORT,
    POPULATION_SORT,
    NUMBER_OF_SORTS
};

enum order_option {
    DESCENDING_ORDER,
    ASCENDING_ORDER,
    NUMBER_OF_ORDERS
};

const char *sort_option_names[] = {"name",
                                   "area",
                                   "population"};
const char *order_option_names[] = {"descending order",
                                    "ascending order"};

typedef struct {
    char region_name[REGION_NAME_MAX + 1];
    double region_area;
    int region_population;
} record;

static struct termios stored_settings;

int get_terminal_lines();

int get_user_choice(enum action current_option, bool *is_exit, bool *is_chosen);

int navigate_list(int current_position, int size, bool *is_exit, bool *is_chosen);

int compare_records(const record *record1, const record *record2,
                    enum sort_option sort_option, enum order_option order_option);

int find_insert_position(const record **data, int size, const record *new_record,
                         enum sort_option sorting_option, enum order_option ordering_option);

void display_menu(enum action current_option, char *opened_file_name, FILE *opened_file);

void create_working_folder(const char *folder_name);

void set_keypress();

void reset_keypress();

void show_files(int current_position, char **files, int size);

void free_records_arr(record **data, int size);

void free_filenames_arr(char **files, int size);

void write_record(FILE *file, record *data);

void create_file();

void create_record(FILE *working_file, char *working_file_name);

void show_records(int current_position, char *working_file_name, int size, record **data);

void read_record(FILE *working_file, char *working_file_name);

void show_sort_options(enum sort_option current_option);

void show_order_options(enum order_option current_option);

void swap_records(record **record1, record **record2);

void sort_records(record **data, int size,
                  enum sort_option sort_option,
                  enum order_option order_option);

bool input_double(double *input);

bool input_int(int *input);

bool is_correct_area(const double *area, double min, double max);

bool is_correct_population(const int *population, double min, double max);

bool file_exists(const char *filepath);

bool string_input(char *line, int max_len);

bool is_valid_filename(const char *filename);

bool is_sorted(record **data, int size, enum sort_option sort, enum order_option order);

bool check_sort_order(record **data, int size,
                      enum sort_option *found_sort_option,
                      enum order_option *found_order_option);

char key_pressed();

char **get_filenames_arr(const char *folder, int *num_of_files);

record **get_records_arr(FILE *working_file, int *size);

FILE *open_file(FILE *opened_file, char **file_name);

FILE *delete_file(FILE *working_file);

FILE *delete_record(FILE *working_file, char *working_file_name);

FILE *edit_record(FILE *working_file, char *working_file_name);

FILE *order_records(FILE *working_file, char *working_file_name);

FILE *insert_record(FILE *working_file, char *working_file_name);


int get_terminal_lines() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        perror("ioctl");
        return -1; // Error
    }
    return ws.ws_row;
}

int get_user_choice(enum action current_option, bool *is_exit, bool *is_chosen) {
    char key;

    key = (char) toupper(key_pressed());

    switch (key) {
        case 'W':
            if (current_option > CREATE_FILE) {
                current_option = (enum action) (current_option - 1);
            }
            break;
        case 'A':
            if (current_option >= CREATE_RECORD && current_option <= DELETE_RECORD) {
                current_option = (enum action) (current_option - 3);
            } else if (current_option >= EDIT_RECORD && current_option <= INSERT_RECORD) {
                current_option = DELETE_RECORD;
            }
            break;
        case 'S':
            if (current_option < INSERT_RECORD) {
                current_option = (enum action) (current_option + 1);
            }
            break;
        case 'D':
            if (current_option >= CREATE_FILE && current_option <= DELETE_FILE) {
                current_option = (enum action) (current_option + 3);
            }
            break;
        case '\n':
            *is_chosen = true;
            break;
        case EXIT_BUTTON:
            *is_exit = true;
        default:
            break;
    }

    return current_option;
}

int navigate_list(int current_position, int size, bool *is_exit, bool *is_chosen) {
    char key;

    key = (char) toupper(key_pressed());

    switch (key) {
        case 'W':
            if (current_position > 0) {
                current_position = current_position - 1;
            }
            break;
        case 'S':
            if (current_position < size - 1) {
                current_position = current_position + 1;
            }
            break;
        case '\n':
            *is_chosen = true;
            break;
        case EXIT_BUTTON:
            *is_exit = true;
            break;
        default:
            break;
    }

    return current_position;
}

int compare_records(const record *record1, const record *record2,
                    enum sort_option sort_option, enum order_option order_option) {
    int result;

    switch (sort_option) {
        case NAME_SORT:
            result = strcmp(record1->region_name, record2->region_name);
            break;
        case AREA_SORT:
            result = (record1->region_area > record2->region_area) -
                     (record1->region_area < record2->region_area);
            break;
        case POPULATION_SORT:
            result = (record1->region_population > record2->region_population) -
                     (record1->region_population < record2->region_population);
            break;
        default:
            printf("Error:" ITALIC_TEXT " Invalid sort option"
                   RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
            result = 0;
    }

    return (order_option == ASCENDING_ORDER) ? result : -result;
}

int find_insert_position(const record **data, int size, const record *new_record,
                         enum sort_option sorting_option, enum order_option ordering_option) {
    int insert_position = 0;

    for (int i = 0; i < size; i++) {
        int comparison_result = compare_records(data[i], new_record, sorting_option, ordering_option);

        if ((ordering_option == ASCENDING_ORDER && comparison_result > 0) ||
            (ordering_option == DESCENDING_ORDER && comparison_result < 0)) {
            break;
        }

        insert_position++;
    }

    return insert_position;
}

void display_menu(enum action current_option, char *opened_file_name, FILE *opened_file) {

    printf(GREEN_TEXT BOLD_TEXT);
    printf("┌───────────────┬─────────────────┐\n");
    printf("│     FILES     │     RECORDS     │\n");
    printf("├───────────────┼─────────────────┤\n");
    printf("│%s Create file%s│%s Create record%s│\n",
           (current_option == CREATE_FILE) ? GREEN_BG BLACK_TEXT "-->" : "",
           (current_option == CREATE_FILE) ? BLACK_BG GREEN_TEXT "" : "   ",
           (current_option == CREATE_RECORD) ? GREEN_BG BLACK_TEXT "-->" : "",
           (current_option == CREATE_RECORD) ? BLACK_BG GREEN_TEXT "" : "   ");
    printf("│%s Open file%s│%s Read record%s│\n",
           (current_option == OPEN_FILE) ? GREEN_BG BLACK_TEXT "-->" : "",
           (current_option == OPEN_FILE) ? "  " BLACK_BG GREEN_TEXT : "     ",
           (current_option == READ_RECORD) ? GREEN_BG BLACK_TEXT "-->" : "",
           (current_option == READ_RECORD) ? "  " BLACK_BG GREEN_TEXT : "     ");
    printf("│%s Delete file%s│%s Delete record%s│\n",
           (current_option == DELETE_FILE) ? GREEN_BG BLACK_TEXT "-->" : "",
           (current_option == DELETE_FILE) ? "" BLACK_BG GREEN_TEXT : "   ",
           (current_option == DELETE_RECORD) ? GREEN_BG BLACK_TEXT "-->" : "",
           (current_option == DELETE_RECORD) ? BLACK_BG GREEN_TEXT "" : "   ");
    printf("│               │%s Edit record%s│\n",
           (current_option == EDIT_RECORD) ? GREEN_BG BLACK_TEXT "-->" : "",
           (current_option == EDIT_RECORD) ? "  " BLACK_BG GREEN_TEXT : "     ");
    printf("│               │%s Order records%s│\n",
           (current_option == ORDER_RECORDS) ? GREEN_BG BLACK_TEXT "-->" : "",
           (current_option == ORDER_RECORDS) ? "" BLACK_BG GREEN_TEXT : "   ");
    printf("│               │%s Insert record%s│\n",
           (current_option == INSERT_RECORD) ? GREEN_BG BLACK_TEXT "-->" : "",
           (current_option == INSERT_RECORD) ? "" BLACK_BG GREEN_TEXT : "   ");
    printf("└───────────────┴─────────────────┘\n");
    printf("\n%s %s", (opened_file == NULL) ? "" : "Current working file:",
           (opened_file == NULL) ? "" : opened_file_name);

    for (int i = 0; i < get_terminal_lines() - MENU_LINES; ++i) {
        printf("\n");
    }

    printf("Use "GREEN_BG BLACK_TEXT"WASD"BLACK_BG GREEN_TEXT" to navigate. Press "
           GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to exit");

}

void create_working_folder(const char *folder_name) {

    if (file_exists(folder_name)) {
        return;
    }

    if (mkdir(folder_name, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
        printf("\nError:" ITALIC_TEXT " Cant create the working folder"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
    }

}

void set_keypress() {
    struct termios new_settings;

    tcgetattr(0, &stored_settings);

    new_settings = stored_settings;

    new_settings.c_lflag &= (~ICANON);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;

    tcsetattr(0, TCSANOW, &new_settings);
}

void reset_keypress() {
    tcsetattr(0, TCSANOW, &stored_settings);
}

void show_files(int current_position, char **files, int size) {
    printf(GREEN_TEXT BOLD_TEXT);

    printf("File list:\n");
    for (int i = 0; i < size; i++) {
        printf("%s %s%s\n", (current_position == i) ? GREEN_BG BLACK_TEXT"-->" : "",
               files[i], (current_position == i) ? BLACK_BG GREEN_TEXT : "");
    }
}

void free_records_arr(record **data, int size) {
    for (int i = 0; i < size; i++) {
        free(data[i]);
    }
    free(data);
}

void free_filenames_arr(char **files, int size) {
    for (int i = 0; i < size; i++) {
        free(files[i]);
    }
    free(files);
}

void create_file() {
    int num_of_files;
    char filename[FILENAME_SIZE];
    char **filenames = get_filenames_arr(working_folder, &num_of_files);
    FILE *file;

    system("clear");

    show_files(NOT_INTERACTIVE, filenames, num_of_files);


    do {
        printf("\nEnter the file name (max %i characters): ", FILENAME_SIZE - 1);
    } while (!string_input(filename, FILENAME_SIZE) || !is_valid_filename(filename));

    char filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(filepath, sizeof(filepath), "%s/%s.txt", working_folder, filename);

    if (file_exists(filepath)) {
        printf("\nFile wasn't created: "ITALIC_TEXT"%s.txt already exists"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT, filename);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return;
    }

    file = fopen(filepath, "w");

    if (file == NULL) {
        printf("\nError:" ITALIC_TEXT " Cant open the file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return;
    }

    system("clear");
    filenames = get_filenames_arr(working_folder, &num_of_files);
    show_files(NOT_INTERACTIVE, filenames, num_of_files);

    printf("\nFile with name "GREEN_BG BLACK_TEXT"%s.txt"BLACK_BG GREEN_TEXT
           " was created successfully!", filename);

    if (fclose(file)) {
        printf("\nError:" ITALIC_TEXT " Can't close the file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
    }

    printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
           "close the program or any other button to return to the menu");
}

bool input_double(double *input) {
    char end_of_input = ' ';
    double value;

    fflush(stdin);

    if (scanf("%lf%c", &value, &end_of_input) && end_of_input == '\n') {
        *input = value;
        return true;
    } else {
        printf("Error:" ITALIC_TEXT "Invalid input. Please try again.\n"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return false;
    }
}

bool input_int(int *input) {
    char end_of_input = ' ';
    int value;

    fflush(stdin);

    if (scanf("%i%c", &value, &end_of_input) && end_of_input == '\n') {
        *input = value;
        return true;
    } else {
        printf("Error:" ITALIC_TEXT "Invalid input. Please try again.\n"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return false;
    }
}

bool is_correct_area(const double *area, double min, double max) {
    if (*area > max) {
        printf("Error:" ITALIC_TEXT " Too large area. Area can't be larger than %lf"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT, max);
        return false;
    }
    if (*area < min) {
        printf("Error:" ITALIC_TEXT " Too small area. Area can't be smaller than %lf"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT, min);
        return false;
    }

    return true;
}

void write_record(FILE *file, record *data) {

    if (file == NULL) {
        printf("\nError:" ITALIC_TEXT " file was not found"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return;
    }

    fprintf(file, "%s %lf %i\n",
            data->region_name, data->region_area, data->region_population);

    fflush(file);
}

bool is_correct_population(const int *population, double min, double max) {
    if (*population > max) {
        printf("Error:" ITALIC_TEXT " Too large population. Population can't be larger than %lf"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT, max);
        return false;
    }
    if (*population < min) {
        printf("Error:" ITALIC_TEXT " Too small population. Population can't be smaller than %lf"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT, min);
        return false;
    }

    return true;
}

bool file_exists(const char *filepath) {
    return !access(filepath, F_OK);
}

char key_pressed() {
    char answer;

    set_keypress();
    answer = (char) getchar();
    reset_keypress();

    return answer;
}

bool string_input(char *line, int max_len) {
    int i = 0;
    char symbol_to_input;

    do {
        symbol_to_input = (char) getchar();

        if (symbol_to_input == '\n') {
            break;
        }

        line[i] = symbol_to_input;
        i++;

    } while (i < max_len);

    if (symbol_to_input != '\n') {
        printf("\nError:" ITALIC_TEXT " Input exceeds the maximum length "
               "of %i characters. Please try again\n"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT, max_len - 1);
        fflush(stdin);
        return false;
    }

    line[i] = '\0';

    return true;
}

bool is_valid_filename(const char *filename) {
    if (filename == NULL || strlen(filename) == 0) {
        printf("\nEmpty input. Please try again\n");
        return false;
    }

    const char *invalid_chars = "\\/:*?\"<>|";
    size_t filename_len = strlen(filename);

    if (filename[0] == '.') {
        printf("\nInvalid filename! Filename cannot start with '.'");
        return false;
    }

    for (size_t i = 0; i < filename_len; i++) {
        for (int j = 0; j < strlen(invalid_chars); j++) {
            if (filename[i] == invalid_chars[j]) {
                printf("\nInvalid filename! Found '%c' at position %zu."
                       "\nDon't use symbols '%s'. Please try again",
                       invalid_chars[j], i + 1, invalid_chars);
                return false;
            }
        }
    }

    if (filename[filename_len - 1] == ' ') {
        printf("\nInvalid filename! Filename cannot end with a space. Please try again");
        return false;
    }

    return true;
}

record **get_records_arr(FILE *working_file, int *size) {
    int capacity = 2, index = 0;

    if (working_file == NULL) {
        printf("Error:" ITALIC_TEXT " No file was opened" RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return NULL;
    }

    fseek(working_file, 0, SEEK_SET);

    record **data = (record **) malloc(capacity * sizeof(record *));

    if (data == NULL) {
        printf("Error:" ITALIC_TEXT " Memory allocation failed" RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return NULL;
    }

    char region_name[REGION_NAME_MAX] = "";
    double region_area = 0;
    int region_population = 0;

    while (fscanf(working_file, "%s %lf %i", region_name, &region_area, &region_population) != EOF) {
        if (index >= capacity) {
            capacity *= 2;
            data = (record **) realloc(data, capacity * sizeof(record *));

            if (data == NULL) {
                printf("Error:" ITALIC_TEXT " Memory reallocation failed" RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
                return NULL;
            }
        }

        data[index] = (record *) malloc(sizeof(record));

        if (data[index] == NULL) {
            printf("Error:" ITALIC_TEXT " Memory allocation failed" RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
            return NULL;
        }

        strncpy(data[index]->region_name, region_name, REGION_NAME_MAX - 1);
        data[index]->region_name[REGION_NAME_MAX - 1] = '\0';

        data[index]->region_area = region_area;
        data[index]->region_population = region_population;

        index++;
    }

    *size = index;

    return data;
}

char **get_filenames_arr(const char *folder, int *num_of_files) {
    int capacity = 2, index = 0;
    DIR *dir;
    struct dirent *entry;

    dir = opendir(folder);

    if (dir == NULL) {
        printf("Error:" ITALIC_TEXT " Cant open the directory"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return NULL;
    }

    char **files = (char **) malloc(capacity * sizeof(char *));

    if (files == NULL) {
        printf("Error:" ITALIC_TEXT " Memory allocation failed"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        if (index >= capacity) {
            capacity *= 2;
            files = (char **) realloc(files, capacity * sizeof(char *));

            if (files == NULL) {
                printf("Error:" ITALIC_TEXT " Memory reallocation failed"
                       RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
                return NULL;
            }
        }

        files[index] = strdup(entry->d_name);

        if (files[index] == NULL) {
            printf("Error:" ITALIC_TEXT " Memory allocation failed"
                   RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
            return NULL;
        }

        index++;
    }

    closedir(dir);
    *num_of_files = index;

    return files;
}

FILE *open_file(FILE *opened_file, char **file_name) {
    int num_of_files, current_position = 0;
    bool is_chosen = false, is_exit = false;
    FILE *file = NULL;

    char **filenames = get_filenames_arr(working_folder, &num_of_files);

    if (num_of_files == 0) {
        system("clear");
        printf("Error:" ITALIC_TEXT " Empty folder"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return NULL;
    }

    do {
        system("clear");
        show_files(current_position, filenames, num_of_files);
        current_position = navigate_list(current_position, num_of_files, &is_exit, &is_chosen);

        if (is_exit) {
            break;
        }

    } while (!is_chosen);

    if (is_exit) {
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return NULL;
    }

    if (opened_file != NULL) {
        fclose(opened_file);
    }

    char filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(filepath, sizeof(filepath), "%s/%s", working_folder, filenames[current_position]);

    if (file_exists(filepath)) {

        file = fopen(filepath, "a+");

        if (file == NULL) {
            printf("\nError:" ITALIC_TEXT " Can't open the file for writing"
                   RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
            return NULL;
        }

    }

    printf("File %s opened successfully!\n", filenames[current_position]);
    *file_name = strdup(filenames[current_position]);

    free_filenames_arr(filenames, num_of_files);

    return file;
}

FILE *delete_file(FILE *working_file) {
    int num_of_files, current_position = 0;
    char **filenames = get_filenames_arr(working_folder, &num_of_files);
    bool is_chosen = false, is_exit = false;


    if (num_of_files == 0) {
        system("clear");
        printf("Error:" ITALIC_TEXT " Empty folder"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return NULL;
    }

    do {
        system("clear");
        show_files(current_position, filenames, num_of_files);
        current_position = navigate_list(current_position, num_of_files, &is_exit, &is_chosen);

        if (is_exit) {
            printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
                   "close the program or any other button to return to the menu");
            return NULL;
        }

    } while (!is_chosen);

    if (is_exit) {
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    char filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(filepath, sizeof(filepath), "%s/%s", working_folder, filenames[current_position]);

    char *deleted_file_name;

    if (file_exists(filepath)) {
        if (!remove(filepath)) {

            deleted_file_name = filenames[current_position];
            filenames = get_filenames_arr(working_folder, &num_of_files);

            system("clear");
            show_files(current_position, filenames, num_of_files);

            printf("\nFile "GREEN_BG BLACK_TEXT"%s"BLACK_BG GREEN_TEXT
                   " has been deleted successfully", deleted_file_name);
            printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
                   "close the program or any other button to return to the menu");
            return NULL;
        } else {
            printf("\nError:" ITALIC_TEXT " Can't delete the file"
                   RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        }
    } else {
        printf("\nError:" ITALIC_TEXT " File was not found"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
    }

    printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
           "close the program or any other button to return to the menu");

    return working_file;
}

void create_record(FILE *working_file, char *working_file_name) {
    int size = 0;
    record input_data;
    record **data;

    if (working_file == NULL) {
        system("clear");
        printf("Error:" ITALIC_TEXT " No file was opened"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return;
    }

    do {
        system("clear");

        data = get_records_arr(working_file, &size);
        show_records(NOT_INTERACTIVE, working_file_name, size, data);


        do {
            printf("\nEnter name of region (max %i characters): ", REGION_NAME_MAX - 1);
        } while (!string_input(input_data.region_name, REGION_NAME_MAX));

        do {
            printf("\nEnter size of region area [%.0lf; %.0lf]: ", area_min, area_max);
        } while (!input_double(&input_data.region_area) ||
                 !is_correct_area(&input_data.region_area, area_min, area_max));

        do {
            printf("\nEnter population of region [%i; %i]: ", population_min, population_max);
        } while (!input_int(&input_data.region_population) ||
                 !is_correct_population(&input_data.region_population, population_min, population_max));

        write_record(working_file, &input_data);

        printf("\nRecord was saved successfully!");
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to stop input "
               "\nor any other button to continue input records\n");

    } while (key_pressed() != EXIT_BUTTON);

    data = get_records_arr(working_file, &size);
    show_records(NOT_INTERACTIVE, working_file_name, size, data);

    printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
           "close the program or any other button to return to the menu");

}

void show_records(int current_position, char *working_file_name, int size, record **data) {

    system("clear");

    if (size == 0) {
        printf("File %s is empty\n", working_file_name);
        return;
    }

    printf("Records in file %s\n\n", working_file_name);
    printf("%-5s%-30s%-20s%-20s\n", "No.", "REGION NAME", "AREA SIZE", "POPULATION");
    for (int i = 0; i < size; i++) {
        printf("%s%-5d%-30s%-20.2lf%-20i%s\n",
               (current_position == i) ? GREEN_BG BLACK_TEXT : "",
               i + 1,
               data[i]->region_name,
               data[i]->region_area,
               data[i]->region_population,
               (current_position == i) ? BLACK_BG GREEN_TEXT : "");
    }
}

void read_record(FILE *working_file, char *working_file_name) {
    int size = 0;

    if (working_file == NULL) {
        system("clear");
        printf("Error:" ITALIC_TEXT " No file was opened"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return;
    }

    record **data = get_records_arr(working_file, &size);

    show_records(NOT_INTERACTIVE, working_file_name, size, data);

    free_records_arr(data, size);

    printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
           "close the program or any other button to return to the menu");
}

FILE *delete_record(FILE *working_file, char *working_file_name) {
    int size = 0, current_position = 0;
    bool is_chosen = false, is_exit = false;


    if (working_file == NULL) {
        system("clear");
        printf("Error:" ITALIC_TEXT " No file was opened"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    record **data = get_records_arr(working_file, &size);

    if (size == 0) {
        system("clear");

        printf("Error:" ITALIC_TEXT " Empty file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    do {
        system("clear");
        show_records(current_position, working_file_name, size, data);
        current_position = navigate_list(current_position, size, &is_exit, &is_chosen);

        if (is_exit) {
            printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
                   "close the program or any other button to return to the menu");
            return working_file;
        }
    } while (!is_chosen);

    if (is_exit) {
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    char temp_filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/%s", working_folder, "temp.txt");

    FILE *temp_file = fopen(temp_filepath, "w");

    if (temp_file == NULL) {
        printf("Error:" ITALIC_TEXT " Can't create temporary file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    record temp_data;

    for (int i = 0; i < size; i++) {
        if (i == current_position) {
            strcpy(temp_data.region_name, data[i]->region_name);
            temp_data.region_area = data[i]->region_area;
            temp_data.region_population = data[i]->region_population;
            continue;
        }
        write_record(temp_file, data[i]);
    }

    fclose(working_file);
    fclose(temp_file);

    char filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(filepath, sizeof(filepath), "%s/%s", working_folder, working_file_name);

    if (rename(temp_filepath, filepath) != 0) {
        printf("Error:" ITALIC_TEXT " Can't rename temporary file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    working_file = fopen(filepath, "a+");

    system("clear");

    data = get_records_arr(working_file, &size);
    show_records(NOT_INTERACTIVE, working_file_name, size, data);

    printf("\nRecord №%i [%s %lf %i] was deleted successfully!",
           current_position + 1, temp_data.region_name,
           temp_data.region_area, temp_data.region_population);
    printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
           "close the program or any other button to return to the menu");

    free_records_arr(data, size);

    return working_file;
}

FILE *edit_record(FILE *working_file, char *working_file_name) {
    int size = 0, current_position = 0;
    bool is_chosen = false, is_exit = false;


    if (working_file == NULL) {
        system("clear");
        printf("Error:" ITALIC_TEXT " No file was opened"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    record **data = get_records_arr(working_file, &size);

    if (size == 0) {
        system("clear");

        printf("Error:" ITALIC_TEXT " Empty file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    do {
        system("clear");
        show_records(current_position, working_file_name, size, data);
        current_position = navigate_list(current_position, size, &is_exit, &is_chosen);

        if (is_exit) {
            printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
                   "close the program or any other button to return to the menu");
            return working_file;
        }
    } while (!is_chosen);

    if (is_exit) {
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    char temp_filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/%s", working_folder, "temp.txt");

    FILE *temp_file = fopen(temp_filepath, "w");

    if (temp_file == NULL) {
        printf("Error:" ITALIC_TEXT " Can't create temporary file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    record input_data;
    record temp_data;

    for (int i = 0; i < size; i++) {
        if (i == current_position) {
            do {
                printf("Enter name of region (max %i characters): ", REGION_NAME_MAX - 1);
            } while (!string_input(input_data.region_name, REGION_NAME_MAX));

            do {
                printf("\nEnter size of region area [%.0lf; %.0lf]: ", area_min, area_max);
            } while (!input_double(&input_data.region_area) ||
                     !is_correct_area(&input_data.region_area, area_min, area_max));

            do {
                printf("\nEnter population of region [%i; %i]: ", population_min, population_max);
            } while (!input_int(&input_data.region_population) ||
                     !is_correct_population(&input_data.region_population,
                                            population_min, population_max));

            strcpy(temp_data.region_name, data[i]->region_name);
            temp_data.region_area = data[i]->region_area;
            temp_data.region_population = data[i]->region_population;

            write_record(temp_file, &input_data);
            continue;
        }
        write_record(temp_file, data[i]);
    }

    fclose(working_file);
    fclose(temp_file);

    char filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(filepath, sizeof(filepath), "%s/%s", working_folder, working_file_name);

    if (rename(temp_filepath, filepath) != 0) {
        printf("Error:" ITALIC_TEXT " Can't rename temporary file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    working_file = fopen(filepath, "a+");

    system("clear");

    data = get_records_arr(working_file, &size);
    show_records(NOT_INTERACTIVE, working_file_name, size, data);

    printf("\nRecord №%i " ITALIC_TEXT "[%s %lf %i]"
           RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT
           " was replaced with record " ITALIC_TEXT "[%s %lf %i]"
           RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT,
           current_position + 1, temp_data.region_name,
           temp_data.region_area, temp_data.region_population,
           input_data.region_name, input_data.region_area, input_data.region_population);
    printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
           "close the program or any other button to return to the menu");

    free_records_arr(data, size);

    return working_file;
}

void show_sort_options(enum sort_option current_option) {

    printf("\nChoose how to sort\n");

    for (int i = 0; i < NUMBER_OF_SORTS; i++) {
        printf("%s by %s%s\n",
               (current_option == i) ? GREEN_BG BLACK_TEXT "-->" : "",
               sort_option_names[i],
               (current_option == i) ? BLACK_BG GREEN_TEXT : "");
    }
}

void show_order_options(enum order_option current_option) {

    printf("\nChoose in which order to sort\n");

    for (int i = 0; i < NUMBER_OF_ORDERS; i++) {
        printf("%s in %s%s\n",
               (current_option == i) ? GREEN_BG BLACK_TEXT "-->" : "",
               order_option_names[i],
               (current_option == i) ? BLACK_BG GREEN_TEXT : "");
    }
}

void swap_records(record **record1, record **record2) {
    record *temp = *record1;
    *record1 = *record2;
    *record2 = temp;
}

void sort_records(record **data, int size,
                  enum sort_option sort_option,
                  enum order_option order_option) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            int comparison_result = compare_records(data[j], data[j + 1], sort_option, order_option);

            if (comparison_result > 0) {
                swap_records(&data[j], &data[j + 1]);
            }
        }
    }
}

bool is_sorted(record **data, int size, enum sort_option sort, enum order_option order) {
    for (int i = 1; i < size; ++i) {
        if (compare_records(data[i - 1], data[i], sort, order) > 0) {
            return false;
        }
    }

    return true;
}

bool check_sort_order(record **data, int size,
                      enum sort_option *found_sort_option,
                      enum order_option *found_order_option) {
    for (int sort = 0; sort < NUMBER_OF_SORTS; ++sort) {
        for (int order = 0; order < NUMBER_OF_ORDERS; ++order) {
            if (is_sorted(data, size, sort, order)) {
                *found_sort_option = sort;
                *found_order_option = order;
                return true;
            }
        }
    }

    return false;
}

FILE *order_records(FILE *working_file, char *working_file_name) {
    int size = 0;
    bool is_chosen_sort = false, is_chosen_order = false, is_exit = false;
    enum sort_option current_sort_option = NAME_SORT;
    enum order_option current_order_option = DESCENDING_ORDER;

    if (working_file == NULL) {
        system("clear");
        printf("Error:" ITALIC_TEXT " No file was opened"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return working_file;
    }

    record **data = get_records_arr(working_file, &size);

    if (size == 0) {
        system("clear");

        printf("Error:" ITALIC_TEXT " Empty file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    do {
        system("clear");

        show_records(NOT_INTERACTIVE, working_file_name, size, data);

        if (!is_chosen_sort) {
            show_sort_options(current_sort_option);
            current_sort_option = navigate_list(current_sort_option, NUMBER_OF_SORTS,
                                                &is_exit, &is_chosen_sort);
        }

        if (is_exit) {
            printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
                   "close the program or any other button to return to the menu");
            return working_file;
        }

        if (is_chosen_sort) {
            system("clear");

            show_records(NOT_INTERACTIVE, working_file_name, size, data);
            show_sort_options(current_sort_option);
            show_order_options(current_order_option);
            current_order_option = navigate_list(current_order_option, NUMBER_OF_ORDERS,
                                                 &is_exit, &is_chosen_order);
        }

        if (is_exit) {
            printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
                   "close the program or any other button to return to the menu");
            return working_file;
        }

    } while (!is_chosen_sort || !is_chosen_order);

    sort_records(data, size, current_sort_option, current_order_option);

    system("clear");

    printf("File was sorted by %s in %s successfully!\nYour updated file:\n",
           sort_option_names[current_sort_option],
           order_option_names[current_order_option]);

    char temp_filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/%s", working_folder, "temp.txt");

    FILE *temp_file = fopen(temp_filepath, "w");

    if (temp_file == NULL) {
        printf("Error:" ITALIC_TEXT " Can't create temporary file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    for (int i = 0; i < size; i++) {
        write_record(temp_file, data[i]);
    }

    fclose(working_file);
    fclose(temp_file);

    char filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(filepath, sizeof(filepath), "%s/%s", working_folder, working_file_name);

    if (rename(temp_filepath, filepath) != 0) {
        printf("Error:" ITALIC_TEXT " Can't rename temporary file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    working_file = fopen(filepath, "a+");

    show_records(NOT_INTERACTIVE, working_file_name, size, data);

    free_records_arr(data, size);

    printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
           "close the program or any other button to return to the menu");

    return working_file;
}

FILE *insert_record(FILE *working_file, char *working_file_name) {
    int size = 0;
    enum sort_option sorting_option;
    enum order_option ordering_option;

    if (working_file == NULL) {
        system("clear");
        printf("Error:" ITALIC_TEXT " No file was opened"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return working_file;
    }

    record **data = get_records_arr(working_file, &size);

    if (size == 0) {
        system("clear");

        printf("Error:" ITALIC_TEXT " Empty file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    if (!check_sort_order(data, size, &sorting_option, &ordering_option)) {
        system("clear");
        printf("Error:" ITALIC_TEXT " Records are not sorted"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        return working_file;
    }


    system("clear");
    show_records(NOT_INTERACTIVE, working_file_name, size, data);
    printf("\nRecords are sorted " GREEN_BG BLACK_TEXT "by %s in %s\n" BLACK_BG GREEN_TEXT,
           sort_option_names[sorting_option],
           order_option_names[ordering_option]);

    char temp_filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/%s", working_folder, "temp.txt");

    FILE *temp_file = fopen(temp_filepath, "w");

    if (temp_file == NULL) {
        printf("Error:" ITALIC_TEXT " Can't create temporary file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    record input_data;

    do {
        printf("\nEnter name of region (max %i characters): ", REGION_NAME_MAX - 1);
    } while (!string_input(input_data.region_name, REGION_NAME_MAX));

    do {
        printf("\nEnter size of region area [%.0lf; %.0lf]: ", area_min, area_max);
    } while (!input_double(&input_data.region_area) ||
             !is_correct_area(&input_data.region_area, area_min, area_max));

    do {
        printf("\nEnter population of region [%i; %i]: ", population_min, population_max);
    } while (!input_int(&input_data.region_population) ||
             !is_correct_population(&input_data.region_population,
                                    population_min, population_max));

    int insert_position = find_insert_position((const record **) data,
                                               size, &input_data,
                                               sorting_option, ordering_option);

    int i;
    for (i = 0; i < size; i++) {
        if (i == insert_position) {
            write_record(temp_file, &input_data);
        }
        write_record(temp_file, data[i]);
    }

    if (i == insert_position) {
        write_record(temp_file, &input_data);
    }

    fclose(working_file);
    fclose(temp_file);

    size++;

    char filepath[FILENAME_SIZE + strlen(working_folder) + 2];
    snprintf(filepath, sizeof(filepath), "%s/%s", working_folder, working_file_name);

    if (rename(temp_filepath, filepath) != 0) {
        printf("Error:" ITALIC_TEXT " Can't rename temporary file"
               RESET_TEXT GREEN_TEXT BLACK_BG BOLD_TEXT);
        printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
               "close the program or any other button to return to the menu");
        return working_file;
    }

    working_file = fopen(filepath, "a+");

    system("clear");
    printf("\n Record was inserted successfully!\n");

    data = get_records_arr(working_file, &size);
    show_records(NOT_INTERACTIVE, working_file_name, size, data);

    printf("\n\nPress "GREEN_BG BLACK_TEXT"ESC"BLACK_BG GREEN_TEXT" to "
           "close the program or any other button to return to the menu");

    free_records_arr(data, size);

    return working_file;
}

int main() {
    bool is_chosen = false, is_exit = false;
    char *working_file_name = NULL;
    enum action current_option = CREATE_FILE;
    FILE *working_file = NULL;

    printf(BLACK_BG);

    create_working_folder(working_folder);

    do {
        do {
            system("clear");
            display_menu(current_option, working_file_name, working_file);
            current_option = (enum action) get_user_choice(current_option, &is_exit, &is_chosen);

            if (is_exit) {
                return 0;
            }

        } while (!is_chosen);

        switch (current_option) {
            case CREATE_FILE:
                create_file();
                break;
            case OPEN_FILE:
                working_file = open_file(working_file, &working_file_name);
                break;
            case DELETE_FILE:
                working_file = delete_file(working_file);
                break;
            case CREATE_RECORD:
                create_record(working_file, working_file_name);
                break;
            case READ_RECORD:
                read_record(working_file, working_file_name);
                break;
            case DELETE_RECORD:
                working_file = delete_record(working_file, working_file_name);
                break;
            case EDIT_RECORD:
                working_file = edit_record(working_file, working_file_name);
                break;
            case ORDER_RECORDS:
                working_file = order_records(working_file, working_file_name);
                break;
            case INSERT_RECORD:
                working_file = insert_record(working_file, working_file_name);
                break;
            default:
                printf("default case\n");
                break;
        }

        is_chosen = false;

    } while (key_pressed() != EXIT_BUTTON);

    if (working_file != NULL) {
        fclose(working_file);
    }

    return 0;
}
