#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>

typedef struct 
{
    char name;
    int x, y;
}Cell;

typedef struct 
{
    Cell *cell;
    int gens;
    int proc_age;
    int food;
}Fox;

typedef struct 
{
    Cell *cell;
    int gens;
    int proc_age;
}Rabbit;

int GEN_PROC_RABBITS;
int GEN_PROC_FOXES;
int GEN_FOOD_FOXES;
int N_GEN;
int R;
int C;
int N;

Rabbit* RABBITS;
Fox* FOXES;

int GEN;

Cell* load_ecosystem(char * argv[]);
void display_ecosystem(Cell system[]);
void rabbit_cell_shift(Rabbit *rabbit, Cell* to);
void fox_cell_shift(Fox *fox, Cell* to);
void move_rabbit(Rabbit *rabbit, Cell* system, Cell* system_copy);
void move_fox(Fox* fox, Cell* system, Cell* system_copy);
int output_ecosystem(Cell* system, char* filename);
void generate_fox(int x, int y,Cell* system);
void generate_rabbit(int x, int y,Cell* system);
void kill_rabbits();
Rabbit* get_rabbit(int x, int y);
Fox* get_fox(int x, int y);
void kill_fox(Fox* fox);
void kill_rabbit(Rabbit* rabbit);
void check_rabbit_overlap();
void check_fox_overlap();
Cell* generate_empty_system(Cell* system);

int main(int argc, char *argv[])
{
    int threads = atoi(argv[1]);
    omp_set_num_threads(threads);

    Cell* system = load_ecosystem(argv);
    Cell* system_copy;

    display_ecosystem(system);
    GEN = 0;
    int max_gen = N_GEN;

    double start = omp_get_wtime();

    while (GEN < max_gen)
    {
        system_copy = generate_empty_system(system);

        printf("Simulating Generation %d...\n", GEN + 1);
        printf("Moving Rabbits...\n");
        //RUN SIMULATION HERE
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < R * C; i++){
            if (RABBITS[i].cell != NULL)
                move_rabbit(&RABBITS[i], system, system_copy);
        }

        printf("Checking Rabbit Overlaps...\n");
        check_rabbit_overlap();

        #pragma omp barrier

        printf("Moving Foxes...\n");
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < R*C; i++)
        {
            if (FOXES[i].cell != NULL)
                move_fox(&FOXES[i], system, system_copy);
        }
        
        #pragma omp barrier

        printf("Checking Fox Overlaps...\n");
        check_fox_overlap();

        printf("Killing eaten Rabbits...\n");
        kill_rabbits();

        printf("Finalizing Generation %d...\n", GEN + 1);
        memcpy(system, system_copy, R*C*sizeof(Cell));
        free(system_copy);

        printf("Gen: %d\n", GEN + 1);

        N_GEN--;
        GEN++;
    }

    double end = omp_get_wtime();

    printf("Configuration: %d %d %d %d %d %d %d\n", GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N);
    display_ecosystem(system);
    output_ecosystem(system, "output");
    printf("Execution Time: %f\n", end - start);
    free(system);
    free(RABBITS);
    free(FOXES);
    return 0;
}

Cell* generate_empty_system(Cell* system){
    Cell* s = calloc(R * C, sizeof(Cell));
    for (int x = 0; x < R; x++)
    {
        for (int y = 0; y < C; y++)
        {
            if (system[y * C + x]. name == '*')
                s[y * C + x].name = '*';
            else
                s[y * C + x].name = '.';

            s[y * C + x].x = x;
            s[y * C + x].y = y;
        }
    }
    return s;
}

void move_rabbit(Rabbit *rabbit, Cell* system, Cell* system_copy){

    int x = rabbit->cell->x;
    int y = rabbit->cell->y;

    int P = 0;
    Cell *north, *south, *east, *west;

    // Assign Neighbour cells
    if (x - 1 < 0)
        north = NULL;
    else
    {
        north = &system[y * C + x - 1];
        
        if (north->name != '*' && north->name != 'F' && north->name != 'R')
        {
            P++;
            north = &system_copy[y * C + x - 1];
        }
        else 
            north = NULL;
    }   
    
    if (x + 1 >= C)
        south = NULL;
    else
    {
        south = &system[y * C + x + 1];
        if (south->name != '*' && south->name != 'F' && south->name != 'R')
        {
            P++;
            south = &system_copy[y * C + x + 1];
        }
        else
            south = NULL;
    }

    if (y + 1 >= R)
        east = NULL;
    else
    {
        east = &system[(y + 1) * C + x];
        if (east->name != '*' && east->name != 'F' && east->name != 'R'){
            P++;
            east = &system_copy[(y + 1) * C + x];
        }
        else 
            east = NULL; 
    }

    
    if (y - 1 < 0)
        west = NULL;
    else
    {
        west = &system[(y - 1) * C + x];
        if (west->name != '*' && west->name != 'F' && west->name != 'R'){
            P++;
            west = &system_copy[(y - 1) * C + x];
        }
        else
            west = NULL;
    }

    Cell *answers[4] = {north, south, east, west};

    if (P > 0)
    {
        //Standard movement logic for both animals
        int counter = (GEN + x + y) % P;

        for (int i = 0; i < 4; i++)
        {
            if (answers[i] != NULL)
            {
                counter--;
                if (counter == 0)
                {
                    rabbit_cell_shift(rabbit, answers[i]);
                    break;
                }
            }
        }

    }

    if (rabbit->proc_age >= GEN_PROC_RABBITS)
    {
        generate_rabbit(x, y, system_copy);
        rabbit->proc_age = 0;
    }

    rabbit->proc_age++;
}

void rabbit_cell_shift(Rabbit *rabbit, Cell* to){
    int x = rabbit->cell->x;
    int y = rabbit->cell->y;
    
    to->name = 'R';
    rabbit->cell = to;

    //printf("Rabbit moved at (%d, %d) to (%d, %d)\n", x, y, to->x, to->y);
}

void fox_cell_shift(Fox *fox, Cell* to){
    to->name = 'F';
    fox->cell = to;

    //printf("Fox moved to (%d, %d)\n", to->x, to->y);
}

void check_rabbit_overlap(){
    int total = C * R;
    int *kill = calloc(total, sizeof(int));  // 0 = keep, 1 = kill

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < total; i++) {
        if (RABBITS[i].cell == NULL) continue;

        for (int j = i + 1; j < total; j++) {
            if (RABBITS[j].cell == NULL) continue;

            if (RABBITS[i].cell->x == RABBITS[j].cell->x &&
                RABBITS[i].cell->y == RABBITS[j].cell->y)
            {
                // decide who dies
                if (RABBITS[i].gens > RABBITS[j].gens)
                    kill[j] = 1;
                else
                    kill[i] = 1;
            }
        }
    }

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < total; i++) {
        if (kill[i])
            kill_rabbit(&RABBITS[i]);
    }

    free(kill);
} 

void check_fox_overlap(){
    int total = C * R;
    int *kill = calloc(total, sizeof(int));  // 0 = keep, 1 = kill

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < C * R; i++)
    {
        if (FOXES[i].cell == NULL) continue;

        for (int j = i + 1; j < C * R; j++)
        {
            if (FOXES[j].cell == NULL) continue;

            if (FOXES[i].cell->x == FOXES[j].cell->x && FOXES[i].cell->y == FOXES[j].cell->y)
            {
                 if (FOXES[i].cell->x == FOXES[j].cell->x && FOXES[i].cell->y == FOXES[j].cell->y){
                    if (FOXES[i].gens > FOXES[j].gens)
                        kill[j] = 1;
                    else if (FOXES[i].gens == FOXES[j].gens)
                    {
                        if (FOXES[i].food > FOXES[j].food)
                            kill[j] = 1;
                        else
                            kill[i] = 1;
                    }
                    else
                        kill[i] = 1;
                }
            }
        }
    }

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < total; i++) {
        if (kill[i])
            kill_fox(&FOXES[i]);
    }

    free(kill);
}

void kill_rabbits(){
    #pragma omp parallel for
    for (int i = 0; i < C * R; i++)
    {
        if (RABBITS[i].cell != NULL && RABBITS[i].cell->name == 'F')  
        {
            kill_rabbit(&RABBITS[i]);
        }   
    }
}

void kill_fox(Fox* fox){
    fox->cell = NULL;
    N--;
}

void kill_rabbit(Rabbit* rabbit){
    rabbit->cell = NULL;
    N--;
}

void generate_rabbit(int x, int y,Cell* system){
    for (int i = 0; i < R * C; i++)
    {
        if (RABBITS[i].cell == NULL)
        {
            system[y * C + x].name = 'R';
            RABBITS[i].cell = &system[y * C + x];
            RABBITS[i].gens = 0;
            RABBITS[i].proc_age = 0;
            N++;

            //printf("A rabbit has been born at (%d, %d).\n", x, y);
            return;
        }
    }
}

void generate_fox(int x, int y,Cell* system){
    for (int i = 0; i < R * C; i++)
    {
        if (FOXES[i].cell == NULL)
        {
            system[y * R + x].name = 'F';
            FOXES[i].cell = &system[y * C + x];
            FOXES[i].gens = 0;
            FOXES[i].proc_age = 0;
            FOXES[i].food = 0;
            N++;
            return;
            
        }
    }
}

void move_fox(Fox* fox, Cell* system, Cell* system_copy){

    int x = fox->cell->x;
    int y = fox->cell->y;

    int P = 0;
    int RP = 0;
    Cell *north, *south, *east, *west;

    // Assign Neighbour cells
    if (x - 1 < 0)
        north = NULL;
    else
    {
        north = &system[y * R + x - 1];
        
        if (north->name == '*' || north->name == 'F')
            north = NULL;
        else if (north->name == 'R')
            RP++;
        else
            P++;

        if (north != NULL){
            north = &system_copy[y * R + x - 1];
        }
    }   

    if (x + 1 >= C)
        south = NULL;
    else
    {
        south = &system[y * R + x + 1];
        if (south->name == '*' || south->name == 'F')
            south = NULL;
        else if (south->name == 'R')
            RP++;
        else
            P++;

        if (south != NULL){
            south = &system_copy[y * R + x + 1];
        }
    }

    if (y + 1 >= R)
        east = NULL;
    else
    {
        east = &system[(y + 1) * R + x];
        if (east->name == '*' || east->name == 'F')
            east = NULL; 
        else if (east->name == 'R')
            RP++;
        else 
            P++;

        if (east != NULL){
            east = &system_copy[(y + 1) * R + x];
        }
    }

    
    if (y - 1 < 0)
        west = NULL;
    else
    {
        west = &system[(y - 1)* R + x];
        if (west->name == '*' || west->name == 'F')
            west = NULL;
        else if (west->name == 'R')
            RP++;
        else
            P++;

        if (west != NULL){
            west = &system_copy[(y - 1) * R + x];
        }
    }

    Cell *answers[4] = {north, south, east, west};

    //Kill Rabbit if possible
    if (P > 0)
    {
        if (RP > 0)
        {
            int counter = (GEN + x + y) % RP;
            for (int i = 0; i < 4; i++)
            {
                if (answers[i] != NULL && answers[i]->name == 'R')
                {
                    if (counter == 0)
                    {
                        fox_cell_shift(fox, answers[i]);
                        fox->food = 0;
                    }
                    counter--;
                }
            }
        
        }
        else{

            if (fox->food >= GEN_FOOD_FOXES)
            {
                kill_fox(fox);
                system_copy[y * R + x].name = '.';
                return;
            }

            //Standard movement logic for both animals
            int counter = (GEN + x + y) % P;

            for (int i = 0; i < 4; i++)
            {
                if (answers[i] != NULL)
                {
                    if (counter == 0)
                    {                    
                        fox_cell_shift(fox, answers[i]);
                    }
                    counter--;
                }
            }
        }
    }
    

    if (fox->proc_age >= GEN_PROC_FOXES)
    {
        generate_fox(x, y, system_copy);
        fox->proc_age = 0;
    }

    fox->food++;
    fox->proc_age++;
}

int output_ecosystem(Cell* system, char* filename){
    FILE *file = fopen(filename, "w");
    if (file == NULL){
        printf("Error opening file for writing.\n");
        return -1;
    }

    fprintf(file, "%d %d %d %d %d %d %d\n", GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N);


    for (int i = 0; i < R * C; i++)
    {
        if (system[i].name == '*')
        {
            fprintf(file, "ROCK %d %d\n", system[i].x, system[i].y);
        }
        else if (system[i].name == 'R')
        {
            fprintf(file, "RABBIT %d %d\n", system[i].x, system[i].y);
        }
        else if (system[i].name == 'F')
        {
            fprintf(file, "FOX %d %d\n", system[i].x, system[i].y);
        }
    }

    fclose(file);
    return 0;
}

Rabbit* get_rabbit(int x, int y){
    for (int i = 0; i < R * C; i++)
    {
        if (RABBITS[i].cell != NULL)
        {
            if (RABBITS[i].cell->x == x && RABBITS[i].cell->y == y)
                return &RABBITS[i];
        }
    }
    return NULL;
}

Fox* get_fox(int x, int y){
    for (int i = 0; i < R * C; i++)
    {
        if (FOXES[i].cell != NULL)
        {
            if (FOXES[i].cell->x == x && FOXES[i].cell->y == y)
                return &FOXES[i];
        }
    }
    return NULL;
}

Cell* load_ecosystem(char * argv[]){

    FILE *file = fopen(argv[2], "r");
    if (file == NULL){
        printf("Error opening file.\n");
        return NULL;
    }
    int config[7];

    for (int i = 0; i < 6; i++)
    {
        fscanf(file, "%d ", &config[i]);
    }
    fscanf(file, "%d\n", &config[6]);

    GEN_PROC_RABBITS = config[0];
    GEN_PROC_FOXES = config[1];
    GEN_FOOD_FOXES = config[2];
    N_GEN = config[3];
    R = config[4];
    C = config[5];
    N = config[6];

    Cell* system = calloc(R * C, sizeof(Cell));

    RABBITS = calloc(R * C, sizeof(Rabbit));

    for (int i = 0; i < C * R; i++)
    {
        RABBITS[i].cell = NULL;
        RABBITS[i].gens = 0;
        RABBITS[i].proc_age = 0;
    }

    FOXES = calloc(R * C, sizeof(Fox));

    for (int i = 0; i < C * R; ++i)
    {
        FOXES[i].cell = NULL;
        FOXES[i].gens = 0;
        FOXES[i].proc_age = 0;
        FOXES[i].food = 0;
    }
    
    int rabbit_count = 0;
    int fox_count = 0;

    char line[50];
    while (fgets(line, sizeof(line), file))
    {
        int x, y;
        char name[10];
        if (sscanf(line, "%s %d %d", name, &x, &y) == 3)
        {
            if (strcmp(name, "ROCK") == 0)
                system[y * C + x].name = '*';
            else
                system[y * C + x].name = name[0];

            if (system[y * C + x].name == 'R')
            {
                RABBITS[rabbit_count].cell = &system[y * C + x];
                rabbit_count++;
            }
            else if (system[y * C + x].name == 'F')
            {
                FOXES[fox_count].cell = &system[y * C + x];
                fox_count++;
            }
            system[y * C + x].x = x;
            system[y * C + x].y = y;
        }
    }

    printf("Configuration: %d %d %d %d %d %d %d\n", GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N);


    for (int x = 0; x < R; x++)
    {
        for (int y = 0; y < C; y++)
        {
            if (system[y * C + x].name == '*' || system[y * C + x].name == 'R' || system[y * C + x].name == 'F')
                continue;

            system[y * C + x].name = '.';
            system[y * C + x].x = x;
            system[y * C + x].y = y;
        }
    }
    
    fclose(file);
    return system;
}

void display_ecosystem(Cell* system){
    for (int i = 0; i < R + 2; i++)
        printf("-");

    printf("\n");
    
    for (int y = 0; y < C; y++)
    {
        printf("|");
        for (int x = 0; x < R; x++)
        {
            if (system[y * C + x].name == '.')
                printf(" ");
            else
                printf("%c", system[y * C + x].name);
        }
        printf("|\n");
    }
    

    for (int i = 0; i < R + 2; i++)
        printf("-");

    printf("\n");
}