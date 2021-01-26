#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>


int Npoints = 10;   //pontok száma a görbén
int popsize = 500; //a populáció mérete
double g = 9.81;
double radians = acos(-1)/180;
double PointMutationRate = 0.02; //pontmutációk esélye
double LongMutationChance = 0.2; //hosszú mutációk esélye
bool logging = true;    //adatok kiírása fájlba
int currentGen = 0;     //aktuális generáció száma
int maxGen = 1000;     //maximális generációszám
double globalMinTime = 1000; //a legrövidebb időt tartalmazó változó, értéke csak placeholder

typedef struct coord{
    double x;
    double y;
} coord;
typedef struct individ{
    coord* curve;        //a görbe pontjait tartalmazó lista
    double time;         //az idő ami alatt végigcsúszna a görbén egy test
    double fitness;      //a görbéhez tartozó fitness érték
} individ;
double dist(coord cd1, coord cd2){     //a két koordináta távolsága
    return (sqrt((cd1.x - cd2.x)*(cd1.x - cd2.x) + (cd1.y - cd2.y)*(cd1.y - cd2.y)));
}
double random(double min, double max){ //random double min és max között
    double range = (max - min);
    double div = RAND_MAX / range;
    return min + (rand() / div);
}
individ newIndivid(coord A, coord B){
    individ self;           //új egyed létrehozása
    self.curve = (coord*)malloc(sizeof(coord) * (Npoints));
    //memória lefoglalása a görbe pontjainak
    self.curve[0] = A;      //a görbe első pontja A pont lesz
    double max = B.y + 800; //az Y koordináták max 800-al lehetnek kisebbek a B pontnál
    double min = A.y;       //az Y koordináták nem lehetnek nagyobbak az A pontnál
    for(int i = 1; i < Npoints-1; i++){
        self.curve[i].x = A.x + (B.x - A.x)/(Npoints-1) * i;
        //az X koordináták a pontok számával egyenlő részre osztják az x távolságot
        self.curve[i].y = random(min, max);
        //az y koordináták megadása a feltételeknek megfelelően
    }
    self.curve[Npoints-1] = B; //a görbe utolsó pontja B
    self.fitness = 0;   //a görbe fitness érték inicializálása, később lesz kiszámítva
    self.time = -1;     //a görbéhez tartozó idő inicializálása, később lesz kiszámítva
    return self;
}
individ* generatePop (coord A, coord B, int popsize, int Npoints){
    individ* pop = (individ*)malloc(sizeof(individ) * popsize);
    //memória lefoglalása a populáció számára
    for(int i = 0; i < popsize; i++)
        pop[i] = newIndivid(A, B);
    //a populáció méretének megfelelő számú egyeddel tölti fel a populációt
    return pop;
}
double calcFitness(individ* self, coord A, coord B){
    double time = 0;    //az idő inicializálása
    double speed = 0;   //kezdősebesség inicializálása
    double dx = (B.x - A.x)/(Npoints);
    //a görbe két pontja közötti X távolság kiszámítása
    for (int i = 1; i < Npoints; i++){
        double dy = self->curve[i].y - self->curve[i-1].y;
        //a görbe két pontja közötti Y távolság kiszámítása
        double s = sqrt(dx*dx + dy*dy);
        //a görbe két pontja közötti teljes távolság kiszámítása
        double a = dy/s * g;
        //a csúszó test gyorsulásának kiszámítása az adott szakaszon
        double t = (-speed + sqrt(speed*speed - 4 * a/2 * -s)) / (2 * a/2);
        //az adott szakaszon való végigcsúszás idejének kiszámítása
        time += t;
        //a szakasz idejének hozzáadása a teljes időhöz
        speed = speed + a * t;
        //a következő szakasz kezdősebességének kiszámítása
    }
    double fitness = 1/pow(time, 3);
    //a fitness érték a teljes idő -3-adik hatánya így minél
    //nagyobb fitness érték minél jobb görbét jelent és kis
    //időbeli különbség nagyobb eltérést eblackményez a fitnessben
    if (fitness != fitness){  //ha a fitness érték nem valós szám,
        self->time = -1;      //mert a görbén nem tudna végighaladni a test
        return 0;             //-1-re állítja az időt, és a fitness 0 lesz
    } else {
        self->time = time;    //beállítja a görbe idejét a kiszámított értékre
        return fitness;       //visszatér a fitness értékével
    }
}
double calcMaxFitness(individ* pop){
    double maxFitness = 0;
    for(int i = 0; i<popsize; i++){
        double f = pop[i].fitness;
        if(f > maxFitness){
            maxFitness = f;
        }
    }
    return maxFitness;
}
void evaluate(individ* pop, coord A, coord B){
    for(int i = 0; i<popsize; i++)
        pop[i].fitness = calcFitness(&pop[i], A, B);
    //a fitness kiértékelése az összes egyedre
    double maxFitness = calcMaxFitness(pop);
    //a maximum fitness kiszámítása
    for(int i = 0; i<popsize; i++)
        pop[i].fitness /= maxFitness;
    //a fitness értékek normalizálása, 0 és 1 közötti értékekre
}
individ select(individ* pop, double maxFitness){
    int failsafe = 0; //ha túllépi a 10000-et, továbbengedi a programot
    while (true){
        int i = rand()%popsize; //kiválaszt egy random egyedet
        double r = random(0, maxFitness); //kiválaszt egy random fitness értéket
        individ self = pop[i];
        if (r < self.fitness || failsafe > 10000)
            return self;
        //Ha a kiválasztott egyed fitnesse nagyobb mint
        //a random érték, az egyed ki lessz választva.
        //Ha kisebb, új egyedet választunk.
        failsafe++;
    }
}
individ crossover(individ parentA, individ parentB, coord A, coord B){
    individ child = newIndivid(A, B);   //leszármazott létrehozása
    int midpoint = random(0, Npoints);  //a törés helyének kiválasztása
    for(int i = 0; i < Npoints; i++){
        if(i < midpoint){
            child.curve[i] = parentA.curve[i];
            //a kiválasztott hely előtti gének bemásolása A szülőből
        } else {
            child.curve[i] = parentB.curve[i];
            //a kiválasztott hely utáni gének bemásolása B szülőből
        }
    }
    return child;
}
individ mutate(individ self, coord A, coord B){
    //pontmutáció
    for(int i = 1; i < Npoints-1; i++){
        if(random(0, 1) < PointMutationRate){
            double max = (B.y-A.y)/(0.5*Npoints);
            self.curve[i].y += random(-max, max);
            //a mutációs aránynak megfelelő eséllyel
            //random értékkel változik az adott pont
        }
    }
    //hosszúmutáció
    if(random(0, 1) < LongMutationChance){
        int a = (int)random(1, Npoints-1-6);
        int b = (int)random(a+6, Npoints-1);
        double r = random(-30, 30);
        for(int i = a; i < b; i++){
            self.curve[i].y += r;
        }
        //random kiválasztott A és B pont között az
        //összes pont azonos, random értékkel változik
    }
    return self;
}
individ writeToFile(individ* pop, char filename[]){ //kiírja egy fájlba az aktuálisgeneráció adatait
    FILE *fp;
    double averageTime = 0;
    int n = 0;
    double minTime = 10000;
    int minIndex;
    for(int i = 0; i < popsize; i++){
        double t = pop[i].time;
        if(t > 0){
            if(t < minTime){
                minTime = t;
                minIndex = i;
            }
            averageTime += t;
            n++;
        }
    }
    averageTime /= n;
    //az átlagos és a legkisebb idő kiszámítása
    if(minTime < globalMinTime)
        globalMinTime = minTime;

    if(logging){
        fp = fopen(filename, "a+");
        fprintf(fp, "%d,%f, %f, %d\n", currentGen, averageTime, minTime, n);
        fclose(fp);
    }
    return pop[minIndex];
}
void generateSVG(individ self, coord A, coord B){ //SVG fájlt generál a kapott görbéből
    FILE* fp;
    fp = fopen("brachistochrone.svg", "w");
    fprintf(fp, "<svg width=\"2000\" height=\"1500\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n");
    fprintf(fp, "<circle cx=\"%f\" cy=\"%f\" r=\"10\" stroke=\"black\" fill=\"black\" />\n", A.x, A.y);
    fprintf(fp, "<circle cx=\"%f\" cy=\"%f\" r=\"10\" stroke=\"black\" fill=\"black\" />\n", B.x, B.y);
    fprintf(fp, "<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke=\"black\" />\n", A.x, A.y, A.x, B.y);
    fprintf(fp, "<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke=\"black\" />\n", A.x, B.y, B.x, B.y);
    fprintf(fp, "<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke=\"black\" />\n", A.x, A.y, B.x, B.y);
    for(int i = 1; i < Npoints; i++){
        fprintf(fp, "<circle cx=\"%f\" cy=\"%f\" r=\"6\" stroke=\"black\" fill=\"black\" />\n", self.curve[i].x, self.curve[i].y);
        fprintf(fp, "<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke=\"black\" />\n", self.curve[i].x, self.curve[i].y, self.curve[i-1].x, self.curve[i-1].y);
    }
    fprintf(fp, "</svg>");
    fclose(fp);
}
int main(int argc, char *argv[]) {
    srand(time(NULL));

    coord A;
    A.x = 50;
    A.y = 50;
    //A pont beállítása
    coord B;
    B.x = 1050;
    B.y = 1050;
    //B pont beállítása
    individ* pop = generatePop(A, B, popsize, Npoints);
    //populáció létrehozása
    if(logging) fclose(fopen("graph.csv", "w"));
    //kiűríti a graph.csv fájlt, ha szeretnénk kiírni az adatokat
    evaluate(pop, A, B);
    individ best = writeToFile(pop, "graph.csv");
    //0. generáció kiértékelése és kiírása

    while(currentGen < maxGen){
        currentGen += 1;
        evaluate(pop, A, B);
        //populáció kiértékelése
        individ* newGen = (individ*)malloc(sizeof(individ) * popsize);
        //memória lefoglalása az új generációnak
        double maxFitness = calcMaxFitness(pop);
        for(int i = 0; i < popsize; i++)
            newGen[i] = mutate(crossover(select(pop, maxFitness), select(pop, maxFitness),
                        A, B), A, B);
        //az új generáció feltöltése a leszármazottakkal
        best = writeToFile(pop, "graph.csv");
        for(int i = 0; i < popsize; i++)
            free(pop[i].curve);
        //előző generáció memóriájának felszabadítása
        free(pop);
        //a popluáció memóriájának felszabadítása
        pop = newGen;
        //az új generáció lesz a populáció
    }
    printf("%f", globalMinTime);
    //az összes egyed közül a leggyorsabb idejének kiírása
    generateSVG(best, A, B);
    //SVG fájl generálása a leggyorsabb egyed pályájából
    for(int i = 0; i < popsize; i++)
        free(pop[i].curve);
    free(pop);
    //lefoglalt memória felszabadítása
    return 0;
}
