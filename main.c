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
void move_rabbit(Rabbit *rabbit, Cell* system);
void move_fox(Fox* fox, Cell* system);
int output_ecosystem(Cell* system, char* filename);
void generate_fox(int x, int y,Cell* system);
void generate_rabbit(int x, int y,Cell* system);
void kill_rabbits();

int main(int argc, char *argv[])
{
    Cell* system = load_ecosystem(argv);
    display_ecosystem(system);

    GEN = 0;

    while (GEN < N_GEN)
    {
        //RUN SIMULATION HERE
        for (int i = 0; i < R * C; i++){
            if (RABBITS[i].cell != NULL)
                move_rabbit(&RABBITS[i], system);
        
            if (FOXES[i].cell != NULL)
                move_fox(&FOXES[i], system);
        }

        kill_rabbits();

        for (int r = 0; r < R * C; r++)
        {
            if (RABBITS[r].cell != NULL)
            {
                printf("RABBIT %d %d\n", RABBITS[r].cell->x, RABBITS[r].cell->y);
            }
        }

        for (int f = 0; f < R * C; f++)
        {
            if (FOXES[f].cell != NULL)
            {
                printf("FOX %d %d\n", FOXES[f].cell->x, FOXES[f].cell->y);
            }
        }

        printf("Gen: %d\n", N_GEN);
        display_ecosystem(system);
        N_GEN--;
    }

    printf("Configuration: %d %d %d %d %d %d %d\n", GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N);
    output_ecosystem(system, "output.txt");
    free(system);
    free(RABBITS);
    free(FOXES);
    return 0;
}

void move_rabbit(Rabbit *rabbit, Cell* system){

    int x = rabbit->cell->x;
    int y = rabbit->cell->y;

    int P = 0;
    Cell *north, *south, *east, *west;

    // Assign Neighbour cells
    if (y - 1 < 0)
        north = NULL;
    else
    {
        north = &system[(y - 1) * R + x];
        
        if (north->name != '*' && north->name != 'F')
            P++;
        else 
            north = NULL;
    }   
    
    if (y + 1 > C)
        south = NULL;
    else
    {
        south = &system[(y + 1) * R + x];
        if (south->name != '*' && south->name != 'F')
            P++;
        else
            south = NULL;
    }

    if (x + 1 > R)
        east = NULL;
    else
    {
        east = &system[y * R + x + 1];
        if (east->name != '*' && east->name != 'F')
            P++;
        else 
            east = NULL; 
    }

    
    if (x - 1 < 0)
        west = NULL;
    else
    {
        west = &system[y * R + x - 1];
        if (west->name != '*' && west->name != 'F')
            P++;
        else
            west = NULL;
    }

    Cell *answers[4] = {north, south, east, west};

    if (P == 0)
    {
        printf("Rabbit at (%d, %d) cannot move.\n", x, y);
    }
    else
    {
        //Standard movement logic for both animals
        int counter = (GEN + x + y) % P;

        for (int i = 0; i < 4; i++)
        {
            if (answers[i] != NULL)
            {
                printf("Checking option (%d, %d)\n", answers[i]->x, answers[i]->y);

                if (counter == 0)
                {
                    rabbit_cell_shift(rabbit, answers[i]);
                    return;
                }
                counter--;
            }
        }

    }

    
}

void rabbit_cell_shift(Rabbit *rabbit, Cell* to){
    int x = rabbit->cell->x;
    int y = rabbit->cell->y;
    
    to->name = 'R';
    rabbit->cell->name = ' ';
    rabbit->cell = to;

    printf("Rabbit moved at (%d, %d) to (%d, %d)\n", x, y, to->x, to->y);
}

void fox_cell_shift(Fox *fox, Cell* to){
    to->name = 'F';
    fox->cell->name = ' ';
    fox->cell = to;

    printf("Fox moved to (%d, %d)\n", to->x, to->y);
}

void kill_rabbits(){
    for (int i = 0; i < C * R; i++)
    {
        if (RABBITS[i].cell != NULL && RABBITS[i].cell->name == 'F')  
        {
            RABBITS[i].cell = NULL;
            N--;
            return;
        }   
    }
}

void kill_fox(Fox* fox){
    fox->cell = NULL;
    N--;
}

void generate_rabbit(int x, int y,Cell* system){
    for (int i = 0; i < R * C; i++)
    {
        if (RABBITS[i].cell == NULL)
        {
            system[y * R + x].name = 'R';
            RABBITS[i].cell = &system[y * R + x];
            RABBITS[i].gens = 0;
            RABBITS[i].proc_age = 0;
            N++;
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
            FOXES[i].cell = &system[y * R + x];
            FOXES[i].gens = 0;
            FOXES[i].proc_age = 0;
            FOXES[i].food = 0;
            N++;
            return;
            
        }
    }
}

void move_fox(Fox* fox, Cell* system){

    int x = fox->cell->x;
    int y = fox->cell->y;

    int P = 0;
    int RP = 0;
    Cell *north, *south, *east, *west;

    // Assign Neighbour cells
    if (y - 1 < 0)
        north = NULL;
    else
    {
        north = &system[(y - 1) * R + x];
        
        if (north->name == '*')
            north = NULL;
        else if (north->name == 'R')
            RP++;
        else
            P++;
    }   
    
    if (y + 1 > R)
        south = NULL;
    else
    {
        south = &system[(y + 1) * R + x];
        if (south->name == '*')
            south = NULL;
        else if (south->name == 'R')
            RP++;
        else
            P++;
    }

    if (x + 1 > C)
        east = NULL;
    else
    {
        east = &system[y * R + x + 1];
        if (east->name == '*')
            east = NULL; 
        else if (east->name == 'R')
            RP++;
        else 
            P++;
    }

    
    if (x - 1 < 0)
        west = NULL;
    else
    {
        west = &system[y * R + x - 1];
        if (west->name == '*')
            west = NULL;
        else if (west->name == 'R')
            RP++;
        else
            P++;
    }

    Cell *answers[4] = {north, south, east, west};

    //Kill Rabbit if possible
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
                    return;
                }
                counter--;
            }
            
        }
        
    }

    //Standard movement logic for both animals
    int counter = (N_GEN + x + y) % P;

    for (int i = 0; i < 4; i++)
    {
        if (answers[i] != NULL)
        {
            if (counter == 0)
            {
                fox_cell_shift(fox, answers[i]);
                return;
            }
            counter--;
        }
    }
}

int output_ecosystem(Cell* system, char* filename){
    FILE *file = fopen(filename, "w");
    if (file == NULL){
        printf("Error opening file for writing.\n");
        return -1;
    }

    fprintf(file, "%d %d %d %d %d %d %d\n", GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N);

    for (int r = 0; r < sizeof(RABBITS) / sizeof(Rabbit); r++)
    {
        if (RABBITS[r].cell != NULL)
        {
            fprintf(file, "RABBIT %d %d\n", RABBITS[r].cell->x, RABBITS[r].cell->y);
        }
    }

    for (int f = 0; f < sizeof(FOXES) / sizeof(Fox); f++)
    {
        if (FOXES[f].cell != NULL)
        {
            fprintf(file, "FOX %d %d\n", FOXES[f].cell->x, FOXES[f].cell->y);
        }
    }

    fclose(file);
    return 0;
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

    Cell* system = malloc(R * C * sizeof(Cell));


    int rabbit_count = 0;
    int fox_count = 0;

    RABBITS = malloc(R * C * sizeof(Rabbit));
    FOXES = malloc(R * C * sizeof(Fox));

    char line[50];
    while (fgets(line, sizeof(line), file))
    {
        int x, y;
        char name[10];
        if (sscanf(line, "%s %d %d", name, &x, &y) == 3)
        {
            if (strcmp(name, "ROCK") == 0)
                system[y * R + x].name = '*';
            else
                system[y * R + x].name = name[0];

            if (system[y * R + x].name == 'R')
            {
                RABBITS[rabbit_count].cell = &system[y * R + x];
                RABBITS[rabbit_count].gens = 0;
                RABBITS[rabbit_count].proc_age = 0;
                rabbit_count++;
            }
            else if (system[y * R + x].name == 'F')
            {
                FOXES[fox_count].cell = &system[y * R + x];
                FOXES[fox_count].gens = 0;
                FOXES[fox_count].proc_age = 0;
                fox_count++;
            }
            system[y * R + x].x = x;
            system[y * R + x].y = y;
        }
    }

    printf("Configuration: %d %d %d %d %d %d %d\n", GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES, N_GEN, R, C, N);


    for (int y = 0; y < C; y++)
    {
        for (int x = 0; x < R; x++)
        {
            if (system[y * R + x].name == '*' || system[y * R + x].name == 'R' || system[y * R + x].name == 'F')
                continue;

            system[y * R + x].name = ' ';
            system[y * R + x].x = x;
            system[y * R + x].y = y;
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
            printf("%c", system[y * R + x].name);
        }
        printf("|\n");
    }
    

    for (int i = 0; i < R + 2; i++)
        printf("-");

    printf("\n");
}