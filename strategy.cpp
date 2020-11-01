#include "strategy.h"
#include "GrafoRRT.h"

Strategy::Strategy(bool time)
{
    L = 0.04; //Distância entre roda e centro
    R = 0.02; //Raio da roda

    vector<double> aux{0,0};

    for(int i = 0; i < 5 ; i++){
       velocidades[i] = 0;
    }

    qtdRobos = 3;
    vrMax = 125;
    Vmax = 2.5;
    Wmax = 62.5;

    for(int i = 0; i < qtdRobos; i++)
    {
        vRL.push_back(aux);
        VW.push_back(aux);
    }

    //Inicialização do vetor de preditor
    int N = 5;
    ballPredPos temp;
    temp.x = 0;
    temp.y = 0;

    for(int i = 0; i < N; i++)
    {
        ballPredMemory.push_back(temp);
    }

    predictedBall = temp;

    //Inicialização dos vetores de memória PID
    int N_mem = 5;

    for(int i=0; i < N_mem; i++)
    {
        memoria_azul_linear.push_back(0);
        memoria_azul_angular.push_back(0);
    }

    //verifica se foi criado um time amarelo
    if(time == true){
        lado = -1;
    }else{//ou time azul
        lado = 1;
    }
}

//Está fazendo muito pouca diferença, talvez deva diminuir o tempo
//de amostragem.
void Strategy::predict_ball(fira_message::Ball ball)
{

    // Depende de quantas foram inicializadas no construtor
    // Retornarei um vetor de 3 pontos, tais que os primeiros elementos são futuros mais próximos
    int N = 0;
    float a = 0;     // Começo da pseudoinversão (A'A)^(-1)
    float theta = 0; //Guarda o termo do preditor
    float temp = 0;

    int futureTime = 50; //Representa quantas iterações no futuro o robô vai predizer a bola
    //Atenção: quanto mais no futuro, mais errado, então imagine esse valor como um ganho de preditor
    //tal que MAIOR = mais pra frente, MENOR = menos pra frente no tempo.

    N = this->ballPredMemory.size()-1; // indice do ultimo elemento

    ballPredPos result; //Resultado será guardado aqui.

    //Atualização do vetor de memória
    ballPredPos ballUpdate;
    ballUpdate.x = ball.x();
    ballUpdate.y = ball.y();
    ballPredMemory.push_back(ballUpdate); //Adicionando novo elemento à memória
    ballPredMemory.erase(ballPredMemory.begin()+0); //Removendo último valor da memória

    //Primeiro para a posição x.
    for (int m = 0; m < (N-1);m++)
       a = a + pow(this->ballPredMemory[m].x,2);
    a = 1/(a + 0.001);

    for(int m = 0; m < (N-1);m++)
       theta = theta + a * this->ballPredMemory[m].x * this->ballPredMemory[m+1].x;
    theta = pow(theta,futureTime);
    result.x = theta * ball.x();

    //Agora pra posicao y.
    a = 0;
    theta = 0;

    for (int m = 0; m < (N-1);m++)
       a = a + pow(this->ballPredMemory[m].y,2);
    a = 1/(a + 0.001);

    for(int m = 0; m < (N-1);m++)
       theta = theta + a * this->ballPredMemory[m].y * this->ballPredMemory[m+1].y;
    theta = pow(theta,futureTime);
    result.y = theta * ball.y();

    //"Retorno" da função.
    this->predictedBall = result;
}

void Strategy::strategy_blue(fira_message::Robot b0, fira_message::Robot b1,fira_message::Robot b2,
                             fira_message::Robot y0, fira_message::Robot y1,fira_message::Robot y2,
                             fira_message::Ball ball, const fira_message::Field & field)
{
    Team blue(b0,b1,b2);
    Team yellow(y0,y1,y2);

    goleiro_petersson(b0,ball,0);
    zagueiro_todos(blue,yellow,ball,1);
    atacante_todos(b2,ball,2);
    cinematica_azul();
}

void Strategy::strategy_yellow(fira_message::Robot y0, fira_message::Robot y1,fira_message::Robot y2,
                               fira_message::Robot b0, fira_message::Robot b1,fira_message::Robot b2,
                               fira_message::Ball ball, const fira_message::Field & field)
{
    Team blue(b0,b1,b2);
    Team yellow(y0,y1,y2);

    goleiro_petersson(y0,ball,0);
    zagueiro_todos(yellow,blue,ball,1);
    //atacante1(y2,ball.x(),ball.y(),2);
    atacante_todos(y2,ball,2);
    cinematica_amarelo();
    //TODO
}

//Vl = (V - WL)/R
//Vr = (V + WL)/R
//Limitando em +- 125
//Resulta em Vmax = 2.5 e Wmax = 62.5

//Calcula as velocidades a serem enviadas ao robô, utilizando cinematica inversa
void Strategy::cinematica_azul()
{
    for(int i = 0; i < qtdRobos; i++)
    {
        vRL[i][0] = (VW[i][0]+VW[i][1]*L)/R;
        vRL[i][1] = (VW[i][0]-VW[i][1]*L)/R;

        vRL[i][0] = limita_velocidade(vRL[i][0],vrMax);
        vRL[i][1] = limita_velocidade(vRL[i][1],vrMax);
    }
}

void Strategy::andarFrente(double vel, int id)
{
    double Vaux = vel/vrMax;
    VW[id][0] = Vmax*Vaux;
    VW[id][1] = 0;
}
void Strategy::cinematica_amarelo()
{
    for(int i = 0; i < qtdRobos; i++)
    {
        vRL[i][0] = (VW[i][0] + VW[i][1]*L)/R;
        vRL[i][1] = (VW[i][0] - VW[i][1]*L)/R;

        vRL[i][0] = limita_velocidade(vRL[i][0],vrMax);
        vRL[i][1] = limita_velocidade(vRL[i][1],vrMax);
    }
}

void Strategy::andarFundo(double vel, int id)
{
    double Vaux = vel/vrMax;
    VW[id][0] = -Vmax*Vaux;
    VW[id][1] = 0;
}

void Strategy::girarHorario(double vel,int id)
{
    double Waux = vel/vrMax;
    VW[id][0] = 0;
    VW[id][1] = Wmax*Waux;
}

void Strategy::girarAntihorario(double vel,int id)
{
    double Waux = vel/vrMax;
    VW[id][0] = 0;
    VW[id][1] = -Waux*Wmax;
}

void Strategy::vaiPara(fira_message::Robot rb, double px, double py, int id)
{
    VW[id][0] = controleLinear(rb,px,py);
    ang_err angulo = olhar(rb, px, py);
    VW[id][1] = controleAngular(angulo.fi);
}

void Strategy::vaiParaDinamico(fira_message::Robot rb, double px, double py, int id)
{
    ang_err angulo = olhar(rb, px, py);
    double erro_angular = angulo.fi; //de -180 a 180
    double erro_linear = angulo.flag*distancia(rb,px,py);

    double V = 0;
    double W = 0;

    double Kp_l = 10;
    double Ki_l = 5;
    double Kd_l = 0.01;

    double Kp_a = 0.15;
    double Ki_a = 0.02;
    double Kd_a = 0.01;

    double temp_integral_l = 0;
    double temp_integral_a = 0;

    for (int i = 0; i < (int)memoria_azul_linear.size(); i++)
    {
        temp_integral_l = temp_integral_l + memoria_azul_linear.at(i);
    }

    for (int i = 0; i < (int)memoria_azul_angular.size(); i++)
    {
        temp_integral_a = temp_integral_a + memoria_azul_angular.at(i);
    }

    V = Kp_l*erro_linear + Ki_l*temp_integral_l + Kd_l*(erro_linear-memoria_azul_linear.at(0));

    W = Kp_a*erro_angular + Ki_a*temp_integral_a + Kd_a*(erro_angular-memoria_azul_angular.at(0));

    VW[id][0] = V;
    VW[id][1] = W;
    atualiza_memoria_azul(erro_linear,erro_angular);
}

void Strategy::atualiza_memoria_azul(double linear, double angular)
{
    //Removendo últimos elementos
    memoria_azul_linear.pop_back();
    memoria_azul_angular.pop_back();

    //Inserindo novos elementos no começo
    memoria_azul_linear.insert(memoria_azul_linear.begin(),linear);
    memoria_azul_angular.insert(memoria_azul_angular.begin(),angular);
}

double Strategy::controleAngular(double fi2) // função testada. lembrete : (sinal de w) = -(sinal de fi)
{
    //double W_max = -0.3;    // constante limitante da tangente hiperbólica : deve ser negativa
    double W_max = Wmax;
    double k_ang = 0.2;
    double Waux = 0;
    double fi = fi2/90; // coloca o erro entre -1 e 1

    Waux = W_max*tanh(k_ang*fi); // nгo linear
                              //  W =  kw*fi;                     // proporcional

    Waux = limita_velocidade(Waux, Wmax); //satura em -1 a 1

    return(Waux); //deve retornar um valor entre -1 e 1
}

double Strategy::controleLinear(fira_message::Robot rb,double px, double py)
{
    double  Vaux = 0;
    double  k_lin = 4;   //constante de contração da tangente hiperbólica Rapha colocou 0.8
    double  V_max = Vmax;       //constante limitante da tangente hiperbólica
    double  v_min = 0.5;  	 //módulo da velocidade linear mínima permitida Rapha colocou 0.03
    double  ang_grande = 30; //para ângulos maiores que esse valor o sistema da prioridade ao W, reduzindo o V
    double  dist = distancia(rb, px, py);

    ang_err angulo = olhar(rb, px, py);

    Vaux = V_max*tanh(k_lin*dist*angulo.flag);  //controle não linear de V

    //if (Vaux*angulo.flag < v_min) Vaux = v_min*angulo.flag;  //aplica o valor definido em v_min

    //if (angulo.fi*angulo.flag > ang_grande) V = v_min*angulo.flag;  // controle de prioridade reduzindo V quando "ang_err" for grande
    Vaux = Vaux*abs(cos(angulo.fi*M_PI / 180));// controle de prioridade reduzindo V quando "ang_err" for grande

    Vaux = limita_velocidade(Vaux, Vmax); //satura em -1 a 1

    return(Vaux);
}

ang_err Strategy::olhar(fira_message::Robot rb, double px, double py)   // função testada - ok!
{
      double r = rb.orientation()*180/M_PI; // orientação do robô de -180 a 180
      ang_err angulo;

      angulo.flag = 1;
      angulo.fi = atan2((py - rb.y()) , (px - rb.x()))*180/M_PI;  //ângulo entre -180 e 180

      if (r < 0)  {r = r + 360;}          //muda para 0 a 360
      if (angulo.fi < 0)   {angulo.fi = angulo.fi + 360;}      //muda para 0 a 360

      angulo.fi = angulo.fi - r;  //erro de orientação a ser corrigido

      if (angulo.fi > 180) {angulo.fi = angulo.fi - 360;}  // limita entre -180 e 180
      if (angulo.fi < -180) {angulo.fi = angulo.fi + 360;} // limita entre -180 e 180

      if (angulo.fi > 90) // se for mais fácil, olhar com o fundo...
      {
          angulo.fi = angulo.fi - 180;
          angulo.flag = -1;
      }
      if (angulo.fi < -90) // se for mais fácil, olhar com o fundo...
      {
          angulo.fi = 180 + angulo.fi;
          angulo.flag = -1;
      }

      // angulo.fi é o ângulo a ser corrigido na convenção do sistema de visão. valores entre -90 e 90.
      // angulo.flag é o sinal a ser aplicado na velocidade linear
      return(angulo);
}

double Strategy::distancia(fira_message::Robot rb, double px, double py)
{
      double dist = sqrt( pow((rb.x()-px),2) + pow((rb.y()-py),2) );
      return(dist);
}

double Strategy::limita_velocidade(double valor, double sat)
{
      if (valor > sat) {valor = sat;}
      if (valor < -sat) {valor = -sat;}
      return(valor);
}

Strategy::~Strategy()
{

}
/* Desnecessário
//Verifica se o robô está perto da parede
bool Strategy::robo_parede(fira_message::Robot rb){
    //limites de x e y
    double lim_x = 0.68;
    double lim_y = 0.58;

    //se o robo estiver longe das paredes retorna falso, caso contrario retorna verdadeiro
    if ((rb.x() <= lim_x) && (rb.x() >= -lim_x)&&(rb.y() <= lim_y) && (rb.y() >= -lim_y)){
        return false;
    }else{
        return true;
    }
}


//Vaipara com saturação nas posições enviadas
void Strategy::vaiPara2(fira_message::Robot rb, double px, double py, int id)
{
    //limites de x e y
    double lim_x = 0.68;
    double lim_y = 0.58;

    //Satura as posições enviadas
    if (px > lim_x)
        px = lim_x;

    if (px < -lim_x)
        px = -lim_x;

    if (py > lim_y)
        py = lim_y;

    if (py < -lim_y)
        py = -lim_y;

    //Calcula a velocidade nas rodas
    VW[id][0] = controleLinear(rb,px,py);
    ang_err angulo = olhar(rb, px, py);
    VW[id][1] = controleAngular(angulo.fi);
}
*/

//Alterações Petersson
//Função de saturação dos valores a serem enviados aos vaipara
void Strategy::saturacao(double V[]){
    //limites de x e y
    double lim_x = 0.68;
    double lim_y = 0.58;

    //Satura as posições enviadas
    if (V[0] > lim_x)
        V[0] = lim_x;

    if (V[0] < -lim_x)
        V[0] = -lim_x;

    if (V[1] > lim_y)
        V[1] = lim_y;

    if (V[1] < -lim_y)
        V[1] = -lim_y;
}

//Atualiza a estrutura de dados contendo as posições de todos os robôs
void Strategy::atualiza_pos(fira_message::Robot b0,fira_message::Robot b1,fira_message::Robot b2,fira_message::Robot y0,fira_message::Robot y1,fira_message::Robot y2){
    Strategy::pos_robos[0][0] = b0.x();
    Strategy::pos_robos[0][1] = b0.y();
    Strategy::pos_robos[1][0] = b1.x();
    Strategy::pos_robos[1][1] = b1.y();
    Strategy::pos_robos[2][0] = b2.x();
    Strategy::pos_robos[2][1] = b2.y();

    Strategy::pos_robos[3][0] = y0.x();
    Strategy::pos_robos[3][1] = y0.y();
    Strategy::pos_robos[4][0] = y1.x();
    Strategy::pos_robos[4][1] = y1.y();
    Strategy::pos_robos[5][0] = y2.x();
    Strategy::pos_robos[5][1] = y2.y();

}


//Campo repulsivo linear
void Strategy::calc_repulsao(fira_message::Robot rb, double F[]){
     double raio_adversario = 0.2;  //raio de detecção do adversario

     F[0] = 0;
     F[1] = 0;

     double dist_adversario;

     for(int i = 0 ; i < 5 ; i++){

         dist_adversario = sqrt(pow((rb.x() - pos_robos[i][0]),2) + pow((rb.y() - pos_robos[i][1]),2));

         if (dist_adversario <= raio_adversario && dist_adversario > 0.01){
             double dist_x = rb.x() - pos_robos[i][0];
             double dist_y = rb.y() - pos_robos[i][1];

             F[0] += raio_adversario*dist_x/dist_adversario - dist_x;
             F[1] += raio_adversario*dist_y/dist_adversario - dist_y;
         }

     }
}

//limita o valor de um vetor
void Strategy::converte_vetor(double V[],double raio){

    double dist = sqrt(pow(V[0],2) + pow(V[1],2));

    if (dist > raio){
        V[0] = raio*V[0]/dist;
        V[1] = raio*V[1]/dist;
    }

}

double Strategy::filtro(double V,int id){

    //filtro IIR
    double k = 0.9;

    V = velocidades[id]*(1-k) + V*k;
    velocidades[id] = V;

    return V;
}

void Strategy::goleiro_petersson(fira_message::Robot rb,fira_message::Ball ball, int id){

    double top_limit = 0.17; //largura do gol/2
    double x_desejado = -0.7*lado;

    double dist = distancia(rb,ball.x(),ball.y());
    vector <double> new_pos = {0,0};

    //se a bola estiver longe utiliza o preditor
    if ( dist > 0.3){
        new_pos = {predictedBall.x,predictedBall.y};
    }else{
        new_pos = {ball.x(),ball.y()};
    }

    if(distancia(rb,x_desejado,rb.y()) >= 0.02){ //se o robô está dentro do retângulo

        if(distancia(rb,x_desejado,rb.y()) >= 0.3){
              vaiPara_desviando(rb,x_desejado,0.0,id);
        }else{
              vaiPara(rb,x_desejado,0.0,id);
        }

    }else{

        ang_err angulo = olhar(rb,rb.x(),top_limit + 5); // calcula diferença entre angulo atual e angulo desejado
        if(angulo.fi >= 0.5 || angulo.fi<= -0.5){ //se o robô não está aproximadamente 90 graus
            andarFrente(0,id);
            VW[id][1] = controleAngular(angulo.fi);
        }

        else if(rb.y() < top_limit && rb.y() < new_pos[1]){ //robô abaixo da bola

            if(angulo.flag == 1){
                andarFrente(125,id);
            }
            else{
                andarFundo(125,id);
            }
        }
        else if(rb.y() > -top_limit && rb.y() > new_pos[1]){ //robô acima da bola
            if(angulo.flag == 1){
                andarFundo(125,id);
            }
            else{
                andarFrente(125,id);
            }
        }
        else{
            andarFrente(0,id);
        }
        //gira se a bola estiver muito perto
        if (distancia(rb,ball.x(),ball.y()) < 0.08){
            if((ball.y() < 0 && lado == 1)){
               girarHorario(125,id);
            }
            if((ball.y() > 0 && lado == -1)){
               girarHorario(125,id);
            }
            if((ball.y() > 0 && lado == 1)){
               girarAntihorario(125,id);
            }
            if((ball.y() < 0 && lado == -1)){
               girarAntihorario(125,id);
            }
        }
    }

}

void Strategy::vaiPara_desviando(fira_message::Robot rb,double px,double py,int id){

    double V[2] = {px - rb.x(),py - rb.y()};

    double F[2] = {0,0};

    calc_repulsao(rb,F);

    double ka = 1;
    double kr = 1;

    converte_vetor(V,0.1);
    converte_vetor(F,0.2);

    double new_pos[2] = {rb.x() + ka*V[0] + kr*F[0],rb.y() + ka*V[1] + kr*F[1]};

    Strategy::saturacao(new_pos);

    vaiPara(rb,new_pos[0],new_pos[1],id);

    filtro(VW[id][0],id);
}

/* Desnecessario
//Verifica se o robo precisa se afastar de outros robos para evitar travamentos
void Strategy::sai_robo(fira_message::Robot rb,fira_message::Robot ry,double F[]){

     double raio_adversario = 1;  //raio de detecção do adversario

     double dist_adversario = sqrt(pow((rb.x() - ry.x()),2) + pow((rb.y() - ry.y()),2)); //calcula distancia ate o adversario

     if (dist_adversario <= raio_adversario){//se a bola estiver longe e o adversario perto
         F[0] += (rb.x() - ry.x())/pow(raio_adversario,2);
         F[1] += (rb.y() - ry.y())/pow(raio_adversario,2);
     }
}

//Verifica se o robo precisa se afastar de outros robos para evitar travamentos
void Strategy::sai_robo2(fira_message::Robot rb,fira_message::Robot ry,double F[]){

     double raio_adversario = 0.2;  //raio de detecção do adversario

     double dist_adversario = sqrt(pow((rb.x() - ry.x()),2) + pow((rb.y() - ry.y()),2)); //calcula distancia ate o adversario
     double dist_x = rb.x() - ry.x();
     double dist_y = rb.y() - ry.y();

     if (dist_adversario <= raio_adversario){//se a bola estiver longe e o adversario perto
         F[0] += raio_adversario*dist_x/dist_adversario - dist_x;
         F[1] += raio_adversario*dist_y/dist_adversario - dist_y;
     }
}
*/

vector<double> Strategy::inserirRRT(vector<double> V_in,vector<double> V_out,int opcao){
    //Se a opção for zero concatena os vetores, senao apaga tudo e insere o novo vetor no lugar
    if (opcao == 0){
        V_out.insert(V_out.end(),V_in.begin(),V_in.end());
    }else{
        V_out.clear();
        V_out.insert(V_out.end(),V_in.begin(),V_in.end());
    }
    return V_out;
}

/*Desnecessario
void Strategy::vaiParaDinamico2(fira_message::Robot rb, double px, double py, int id)
{
    //limites de x e y
    double lim_x = 0.68;
    double lim_y = 0.58;

    //Satura as posições enviadas
    if (px > lim_x)
        px = lim_x;

    if (px < -lim_x)
        px = -lim_x;

    if (py > lim_y)
        py = lim_y;

    if (py < -lim_y)
        py = -lim_y;

    ang_err angulo = olhar(rb, px, py);
    double erro_angular = angulo.fi; //de -180 a 180

    double dist = distancia(rb,px,py);

    double erro_linear = angulo.flag*dist;/////

    double V = 0;
    double W = 0;

//    double Kp_l = 5;
  //  double Ki_l = 10;
  //  double Kd_l = 1;

    double Kp_l = 1;
    double Ki_l = 1;
    double Kd_l = 1;

  //  double Kp_a = 0.15;
  //  double Ki_a = 0.02;
  //  double Kd_a = 0.01;

    double Kp_a = 0.2;
    double Ki_a = 0.5;
    double Kd_a = 0.1;

    double temp_integral_l = 0;
    double temp_integral_a = 0;

    for (int i = 0; i < (int)memoria_azul_linear.size(); i++)
    {
        temp_integral_l = temp_integral_l + memoria_azul_linear.at(i);
    }

    for (int i = 0; i < (int)memoria_azul_angular.size(); i++)
    {
        temp_integral_a = temp_integral_a + memoria_azul_angular.at(i);
    }

    V = Kp_l*erro_linear + Ki_l*temp_integral_l + Kd_l*(erro_linear-memoria_azul_linear.at(0));

    W = Kp_a*erro_angular + Ki_a*temp_integral_a + Kd_a*(erro_angular-memoria_azul_angular.at(0));

    double sat = 0.8;

    if (V > 0 && V< sat){
        V = sat;
    }
    if (V < 0 && V>-sat){
        V = -sat;
    }

    VW[id][0] = V;
    VW[id][1] = W;

    atualiza_memoria_azul(erro_linear,erro_angular);
}
*/
/*
// goleiro de David
void Strategy::goleiro(fira_message::Robot rb,double xbola,double ybola,int id){

  double top_limit = 0.4/2; //largura do gol/2
  double x_desejado = -1.4/2.0;

  if(distancia(rb,x_desejado,rb.y()) >= 0.02){ //se o robô está dentro do retângulo
      vaiPara(rb,x_desejado,0.0,id);
       printf("Entrou aqui/n");
    }
  else{

      ang_err angulo = olhar(rb,rb.x(),top_limit + 5); // calcula diferença entre angulo atual e angulo desejado
      printf("%i\n", angulo.flag);
      if(angulo.fi >= 0.5 || angulo.fi<= -0.5){ //se o robô não está aproximadamente 90 graus
          andarFrente(0,id);

          VW[id][1] = controleAngular(angulo.fi);

      }

      else if(rb.y() < top_limit && rb.y() < ybola){ //robô abaixo da bola

          if(angulo.flag == 1){
              andarFrente(100,id);
              printf("A\n");
          }
          else{
              andarFundo(100,id);
               printf("B\n");
          }
      }
      else if(rb.y() > -top_limit && rb.y() > ybola){ //robô acima da bola
          if(angulo.flag == 1){
              andarFundo(100,id);
               printf("C\n");
          }
          else{
              andarFrente(100,id);
               printf("D\n");
          }
      }
      else{
          andarFrente(0,id);
      }
      if(distancia(rb,xbola,ybola) < 0.1){ //robô proóximo da bola
          chute(id);
      }
  }

}
*/
/*
void Strategy::chute(int id){
    VW[id][1] = -125;
}
*/
/* Não funciona mais com as mudanças
void Strategy::vaiPara_hotwheels(fira_message::Robot b0, fira_message::Robot b1,fira_message::Robot b2,
                                 fira_message::Robot y0, fira_message::Robot y1,fira_message::Robot y2,
                                 double px, double py,int id){

    //limites de x e y
    double lim_x = 0.68;
    double lim_y = 0.58;

    //Satura as posições enviadas
    if (px > lim_x)
        px = lim_x;

    if (px < -lim_x)
        px = -lim_x;

    if (py > lim_y)
        py = lim_y;

    if (py < -lim_y)
        py = -lim_y;

    double V[2];
    double F[2]= {0,0};
    double new_pos[2];
    double ka = 1;
    double kr = 1;

    if(id == 0){

        double V[2] = {px - b0.x(),py - b0.y()};



        sai_robo2(b0,y0,F);
        sai_robo2(b0,y1,F);
        sai_robo2(b0,y2,F);

        sai_robo2(b0,b1,F);
        sai_robo2(b0,b2,F);

        converte_vetor(V,0.1);
        converte_vetor(F,0.2);


        double new_pos[2] = {b0.x() + ka*V[0] + kr*F[0],b0.y() + ka*V[1] + kr*F[1]};

        vaiPara2(b0,new_pos[0],new_pos[1],id);

        VW[id][0] = filtro(VW[id][0]);

    }
        else{

            if (id == 1){

                double V[2] = {px - b1.x(),py - b1.y()};



                sai_robo2(b1,y0,F);
                sai_robo2(b1,y1,F);
                sai_robo2(b1,y2,F);

                sai_robo2(b1,b0,F);
                sai_robo2(b1,b2,F);

                converte_vetor(V,0.1);
                converte_vetor(F,0.2);


                double new_pos[2] = {b1.x() + ka*V[0] + kr*F[0],b1.y() + ka*V[1] + kr*F[1]};

                vaiPara2(b1,new_pos[0],new_pos[1],id);

                VW[id][1] = filtro(VW[id][1]);


            }
            else{



                double V[2] = {px - b2.x(),py - b2.y()};



                sai_robo2(b2,y0,F);
                sai_robo2(b2,y1,F);
                sai_robo2(b2,y2,F);

                sai_robo2(b2,b0,F);
                sai_robo2(b2,b1,F);

                converte_vetor(V,0.1);
                converte_vetor(F,0.2);


                double new_pos[2] = {b2.x() + ka*V[0] + kr*F[0],b2.y() + ka*V[1] + kr*F[1]};

                vaiPara2(b2,new_pos[0],new_pos[1],id);

                VW[id][2] = filtro(VW[id][2]);



            }
        }
}
*/

// Zagueiro David
void Strategy::zagueiro(fira_message::Robot rb, double xbola, double ybola, int id){
   double x_penalti =  0.4;
   double x_meio_de_campo = 0.0;
   double x_radius = 0.2;
   double y_top = 0.35;
   if(xbola >= x_penalti){
       vaiPara(rb,x_meio_de_campo,ybola,id);
   }else if(xbola >= x_meio_de_campo){
       vaiPara(rb,-x_radius,ybola,id);
   }else if(xbola >= -x_penalti){
       vaiPara(rb,xbola,ybola,id);
   }else if(ybola >= y_top && rb.y() <= ybola){
       vaiPara(rb,xbola,ybola,id);
   }else if(ybola <= -y_top && rb.y() >= ybola){
       vaiPara(rb,xbola,ybola,id);
   }else{
       vaiPara(rb,-x_penalti -0.1, 0.0,id);
   }
}

// Zagueiro David + Cone
void Strategy::zagueiro2(fira_message::Robot rb, double xbola, double ybola, int id){
    double x_penalti =  0.4;
    double x_meio_de_campo = 0.0;
    double x_radius = 0.2;
    double y_top = 0.65;
    double ala_deepth = 0.3;
    double K_press = 0.2;

    if(xbola > x_penalti)
    {    //Se a Bola estiver na zona "A"
        vaiPara(rb,x_meio_de_campo + K_press + 0.1,ybola,id);
        printf("AAA !! \n");
    }
    else if(xbola >= x_meio_de_campo
            && (ybola < (y_top - ala_deepth)
               && ybola > (ala_deepth - y_top)))
    {    //Se a Bola estiver na zona "B_mid"
        vaiPara(rb,-x_penalti,ybola,id);
        printf("BBB mid !! \n");
    }
    else if((xbola >= x_meio_de_campo)
             && (ybola > (y_top - ala_deepth) || ybola < (ala_deepth - y_top)))
    {    //Se a Bola estiver na zona "B_top" ou "B_bot"
        vaiPara(rb,-x_radius,ybola,id);
        printf("BBB top ou bot !! \n");
    }
    else if(xbola < x_meio_de_campo
            && xbola > rb.x()
              && (ybola < (y_top - ala_deepth)
                && ybola > (ala_deepth - y_top)))
    {    //Se a Bola estiver na zona "C"
        vaiPara(rb,xbola,ybola,id);
        printf("CCC !! \n");
    }
    else if((xbola < x_meio_de_campo
             && ybola > (y_top - ala_deepth)))
    {    //Se a Bola estiver na zona "D"
        vaiPara(rb,xbola,y_top - ala_deepth,id);
        printf("DDD !! \n");
    }
    else if((xbola < x_meio_de_campo)
             && ybola < (ala_deepth - y_top))
    {    //Se a Bola estiver na zona "E"
        vaiPara(rb,xbola,ala_deepth - y_top,id);
        printf("EEE !! \n");
    }
    else
    {
        printf("tururu !! \n");
        vaiPara(rb,-x_penalti -0.1, 0.0,id);
    }

   // if((distancia(rb,xbola,ybola) < 0.08) && (xbola > rb.x())){
   //     chute(id);
   // }
}

void Strategy::zagueiro3(fira_message::Robot rb, double xbola, double ybola, int id){
   double x_penalti =  0.4;
   double x_meio_de_campo = 0.0;
   double x_radius = 0.2;
   double y_top = 0.35;
   if(xbola >= x_penalti){
       vaiPara_desviando(rb,x_meio_de_campo,ybola,id);
   }else if(xbola >= x_meio_de_campo){
       vaiPara_desviando(rb,-x_radius,ybola,id);
   }else if(xbola >= -x_penalti){
       vaiPara_desviando(rb,xbola,ybola,id);
   }else if(ybola >= y_top && rb.y() <= ybola){
       vaiPara_desviando(rb,xbola-0.2,y_top+0.1,id);
   }else if(ybola <= -y_top && rb.y() >= ybola){
       vaiPara_desviando(rb,xbola-0.2,y_top+0.1,id);
   }else{
       vaiPara_desviando(rb,-x_penalti -0.1, 0.0,id);
   }
}

void Strategy::atacante1(fira_message::Robot rb, double px, double py, int id){

    double Cx = -0.05*lado;
    double Cy = 0.05;
    double destino = -0.8*lado;

    if (lado == 1){
        if (px >= 0){
            if(distancia(rb,px,py) > 0.07){
                double V[2] = {px+Cx,py*(1+Cy)};
               // double V[2] = {px,py};
                saturacao(V);
                vaiPara_desviando(rb,V[0],V[1],id);
            }else{
           //     calc_repulsao2(rb,px,py,F);
                vaiPara(rb,destino,0,id);
            }
        }else{
            vaiPara(rb,0,py*(1+Cy),id);
        }
    }else{
        if (px <= 0){
            if(distancia(rb,px,py) > 0.07){
                double V[2] = {px+Cx,py*(1+Cy)};
               // double V[2] = {px,py};
                saturacao(V);
                vaiPara_desviando(rb,V[0],V[1],id);
            }else{
           //     calc_repulsao2(rb,px,py,F);
                vaiPara(rb,destino,0,id);
            }
        }else{
            vaiPara(rb,0,py*(1+Cy),id);
        }
    }

}

void Strategy::vaiPara_diodo(fira_message::Robot rb,double px, double py, int id){
    if (lado > 0){
        if (rb.x() > px){
            vaiPara(rb,px,py,id);
        }else{
            State inicio(rb.x(),rb.y(),rb.orientation());
            State fim(px,py,rb.orientation());
            rrt_graph rrt(inicio,fim);

        }
    }
}


//Atacante de Lázaro e cone
void Strategy::atacante_todos(fira_message::Robot rb,fira_message::Ball ball, int id)
{

    double alpha = M_PI/8;
    double meioGolx = -0.75*lado;
    double beta = atan2(rb.x() - meioGolx , rb.y());
    int K = round(beta/alpha);
    double gain;
  //  double resultante_2 = {0,0};

    if(K<2)
        gain = 0.25;
    else if(K>=2 && K <= 3)
        gain = 0.15;
    else if(K>3 && K < 5)
        gain = 0.30;
    else if(K>=5 && K < 7)
        gain = 0.15;
    else
        gain = 0.25;

    delete resultante_2;
    resultante_2 = new vector<double>();

    //Determinação da origem do vetor resultante
    resultante_2->push_back(rb.x());
    resultante_2->push_back(rb.y());

    //distância atacante-bola
    double dist =  sqrt(pow(ball.x()-rb.x(),2.0)+pow( ball.y()-rb.y(),2.0));

    double componenteX;
    double componenteY;

    double k = dist;
    if(dist < 0.05)
        k  = 0.05;

    (*resultante_2)[0]+=-k*sin(atan2((rb.x() - ball.x()),(rb.y() - ball.y())));
    (*resultante_2)[1]+=-k*cos(atan2((rb.x() - ball.x()),(rb.y() - ball.y())));

    if(lado > 0){//azul
        if((dist < 0.08) && (ball.x() >= rb.x())){
            (*resultante_2)[0]+=-gain*sin(beta);
            (*resultante_2)[1]+=-gain*cos(beta);
        }
        if((ball.x() < 0.5 && ball.y() < -0.20)&& (rb.y() > ball.y())&&(rb.x() < ball.x())/*&&(abs(ball.y()-blue[id].x())>0.07)*/)
        {
            //Vetor de corrção
            componenteX = /*ball.x()*/ -  0.1*sin(atan2(ball.x() - meioGolx,ball.y() - 0.0)+M_PI);
            componenteY = /*ball.y()*/ -  0.1*cos(atan2(ball.x() - meioGolx,ball.y() - 0.0)+M_PI);

            (*resultante_2)[0]+=componenteX;
            (*resultante_2)[1]+=componenteY;
        }

        if((ball.x() < 0.5 && ball.y() > 0.20)&&(rb.y() < ball.y())&&(rb.x() < ball.x())/*&&(abs(ball.y()-blue[id].x())>0.07)*/)
        {
            //Vetor de corrção
            componenteX = /*ball.x()*/ -  0.1*sin(atan2(ball.x() - meioGolx,ball.y() - 0.0)+M_PI);
            componenteY = /*ball.y()*/ -  0.1*cos(atan2(ball.x() - meioGolx,ball.y() - 0.0)+M_PI);

            (*resultante_2)[0]+=componenteX;
            (*resultante_2)[1]+=componenteY;
        }

        if((ball.x() < rb.x()))
        {
            int a = 0;
            int b = 1;
            //Consideração do vetor de predição
            if(predictedBall.x < ball.x() && (abs(ball.y()-rb.y()) > 0.1))
                a = 1;
            if(dist > 0.35)
                b = 0;

            double repulsivoX;
            double repulsivoY;

            if(ball.y() - rb.y()>0)
            {
                repulsivoX = 0.1*sin(M_PI*0.25);
                repulsivoY = 0.1*cos(M_PI*0.25);
            }else
            {
                repulsivoX = 0.1*sin(M_PI*0.75);
                repulsivoY = 0.1*cos(M_PI*0.75);
            }

            //Componentes do Vetor direção
            double angle = atan2(ball.x()-predictedBall.x,ball.y()-predictedBall.y);
            componenteX = /*blue[id].x()*/ -  a*0.1*sin(angle) - b*repulsivoX;
            componenteY = /*blue[id].y()*/ -  a*0.1*cos(angle) - b*repulsivoY;

            (*resultante_2)[0]+= componenteX;
            (*resultante_2)[1]+= componenteY;
        }

        if(ball.x()==0)
        {
            vaiPara(rb,ball.x(),ball.y(),id);
        }
        else
        {
            if(ball.x()<=0)
                vaiPara_desviando(rb,0,(*resultante_2)[1],id);
            else
                vaiPara_desviando(rb,(*resultante_2)[0],(*resultante_2)[1],id);
        }
    }else{
        if((dist < 0.08) && (ball.x() <= rb.x())){
            (*resultante_2)[0]+=-gain*sin(beta);
            (*resultante_2)[1]+=-gain*cos(beta);
        }
        if((ball.x() > -0.5 && ball.y() < -0.20)&& (rb.y() > ball.y())&&(rb.x() > ball.x())/*&&(abs(ball.y()-blue[id].x())>0.07)*/)
        {
            //Vetor de corrção
            componenteX = /*ball.x()*/ -  0.1*sin(atan2(ball.x() - meioGolx,ball.y() - 0.0)+M_PI);
            componenteY = /*ball.y()*/ -  0.1*cos(atan2(ball.x() - meioGolx,ball.y() - 0.0)+M_PI);

            (*resultante_2)[0]+=componenteX;
            (*resultante_2)[1]+=componenteY;
        }

        if((ball.x() > -0.5 && ball.y() > 0.20)&&(rb.y() < ball.y())&&(rb.x() > ball.x())/*&&(abs(ball.y()-blue[id].x())>0.07)*/)
        {
            //Vetor de corrção
            componenteX = /*ball.x()*/ -  0.1*sin(atan2(ball.x() - meioGolx,ball.y() - 0.0)+M_PI);
            componenteY = /*ball.y()*/ -  0.1*cos(atan2(ball.x() - meioGolx,ball.y() - 0.0)+M_PI);

            (*resultante_2)[0]+=componenteX;
            (*resultante_2)[1]+=componenteY;
        }

        if((ball.x() > rb.x()))
        {
            int a = 0;
            int b = 1;
            //Consideração do vetor de predição
            if(predictedBall.x > ball.x() && (abs(ball.y()-rb.y()) > 0.1))
                a = 1;
            if(dist > 0.35)
                b = 0;

            double repulsivoX;
            double repulsivoY;

            if(ball.y() - rb.y()>0)
            {
                repulsivoX = 0.1*sin(M_PI*0.25);
                repulsivoY = 0.1*cos(M_PI*0.25);
            }else
            {
                repulsivoX = 0.1*sin(M_PI*0.75);
                repulsivoY = 0.1*cos(M_PI*0.75);
            }

            //Componentes do Vetor direção
            double angle = atan2(ball.x()-predictedBall.x,ball.y()-predictedBall.y);
            componenteX = -a*0.1*sin(angle) - b*repulsivoX;
            componenteY = -a*0.1*cos(angle) - b*repulsivoY;

            (*resultante_2)[0]+= componenteX;
            (*resultante_2)[1]+= componenteY;
        }

        if(ball.x()==0)
        {
            vaiPara(rb,ball.x(),ball.y(),id);
        }
        else
        {
            if(ball.x()>=0)
                vaiPara_desviando(rb,0,(*resultante_2)[1],id);
            else
                vaiPara_desviando(rb,(*resultante_2)[0],(*resultante_2)[1],id);
        }
    }
}

//Zagueiro de cone com a ideia de predição potencial de antecipação usando uma espécie de "Motor Schema"
void Strategy::zagueiro_todos(Team time1, Team time2, fira_message::Ball ball, int id)
{
    double raio = 0.08;
    double angle;
    double dist;
    double gain = 3*raio;  //Ganho inicial, dependência da região do campo
    double v_of;
    int k = 0;

    double dist_2 = sqrt(pow(ball.x() - time1[id].x(),2.0)+pow( ball.y() - time1[id].y(),2.0));

//if (lado > 0){
    if(ball.x() - time1[id].x() < 0)
    {
       v_of = ball.x() - time1[id].x();
       k = 1;
    }
    else
    {
        v_of = 0;
        k = 0;
    }

    //Vetor que aponta na direção do ponto predito da posição da bola com um fator "v_of"
    if(dist_2 > 0.2 && k == 1){
        angle = atan2(time1[id].x() - (predictedBall.x + v_of),time1[id].y() - predictedBall.y);
    }else{
        angle = atan2(time1[id].x() - ball.x(),time1[id].y() - ball.y());
    }

    double componenteX = time1[id].x() -  gain*sin(angle);
    double componenteY = time1[id].y() -  gain*cos(angle);

    delete resultante;
    resultante = new vector<double>();

    //Usando vetor do zaguairo ao ponto predito, para composição
    resultante->push_back(componenteX);
    resultante->push_back(componenteY);

    //Usando o vetor de predição com origem no zagueiro, para composição

    (*resultante)[0]+=-k*(-predictedBall.x + ball.x());
    (*resultante)[1]+=-k*(-predictedBall.y + ball.y());

    //Repulsão da área do goleiro
    if(time1[id].x()<-0.40 && (time1[id].y()<0.3 && time1[id].y()>-0.30) && ball.x()>-0.6)
    {
        (*resultante)[0]+=-2*(-0.40-(time1[id].x()))*sin(atan2(time1[id].x() - ball.x(),time1[id].y() - ball.y()));
        (*resultante)[1]+=-2*(-0.40-(time1[id].x()))*cos(atan2(time1[id].x() - ball.x(),time1[id].y() - ball.y()));
    }
    //Repulsão da bola no retorno do zagueiro
    if(k==1 && dist_2<0.2)
    {
        (*resultante)[0]+=-gain*sin(M_PI/2);
        (*resultante)[1]+=-gain*cos(M_PI/2);
    }

    double min_dist = 999;

    for(int i = 0;i < 3;i++)
    {
        angle = atan2(time2[i].x() - ball.x(),time2[i].y() - ball.y());
        dist = sqrt(pow(ball.x() - time2[i].x(),2.0)+pow( ball.y() - time2[i].y(),2.0));

        if(dist<0.15 && dist > 0.1)
            gain = 4*(0.15 - dist);
        else if(dist < 0.1)
            gain = 0.2;
        else
            gain = 0.0;

        if(min_dist>dist)
            min_dist = dist;

        componenteX = time1[id].x()-gain*sin(angle);
        componenteY = time1[id].y()-gain*cos(angle);

        //Usando o vetor Adversário-bola com um ganho "gain", para composição
        (*resultante)[0]+=-gain*sin(angle);
        (*resultante)[1]+=-gain*cos(angle);

    }


    if(((k==0 && dist_2 < 0.15)||((k==0 && dist_2/min_dist < 1.3)&&time1[id].x()<0)||((k==0 && dist_2/min_dist < 0.6)&&time1[id].x()>=0))
            &&!(time1[id].x()<-0.5 && (time1[id].y()<0.35 && time1[id].y()>-0.35)))
    {
        vaiPara_desviando(time1[id],ball.x(),ball.y(),id);
    }else{
        if(time1[id].x()<-0.50 && (time1[id].y()<0.35 && time1[id].y()>-0.35))
        {
                vaiPara_desviando(time1[id],-0.55,(*resultante)[1],id);
        }
        else
        {
            if(ball.x()<-0.5 && (ball.y()<0.3 && ball.y()>-0.30) && k==1)
                vaiPara_desviando(time1[id],-0.4,0.0,id);
            else if(ball.x()>0)
                vaiPara_desviando(time1[id],0.0,(*resultante)[1],id);
            else
                vaiPara_desviando(time1[id],(*resultante)[0],(*resultante)[1],id);
        }


    }

}
/*else{

    if(ball.x() - time1[id].x() > 0)
    {
       v_of = ball.x() - time1[id].x();
       k = 1;
    }
    else
    {
        v_of = 0;
        k = 0;
    }

    //Vetor que aponta na direção do ponto predito da posição da bola com um fator "v_of"
    if(dist_2 > 0.2 && k == 1){
        angle = atan2(time1[id].x() - (predictedBall.x + v_of),time1[id].y() - predictedBall.y);
    }else{
        angle = atan2(time1[id].x() - ball.x(),time1[id].y() - ball.y());
    }

    double componenteX = time1[id].x() -  gain*sin(angle);
    double componenteY = time1[id].y() -  gain*cos(angle);

    delete resultante;
    resultante = new vector<double>();

    //Usando vetor do zaguairo ao ponto predito, para composição
    resultante->push_back(componenteX);
    resultante->push_back(componenteY);

    //Usando o vetor de predição com origem no zagueiro, para composição

    (*resultante)[0]+=-k*(-predictedBall.x + ball.x());
    (*resultante)[1]+=-k*(-predictedBall.y + ball.y());

    //Repulsão da área do goleiro
    if(time1[id].x() > 0.40 && (time1[id].y()<0.3 && time1[id].y()>-0.30) && ball.x() < 0.6)
    {
        (*resultante)[0]+=-2*((time1[id].x())-0.40)*sin(atan2(time1[id].x() - ball.x(),time1[id].y() - ball.y()));
        (*resultante)[1]+=-2*((time1[id].x())-0.40)*cos(atan2(time1[id].x() - ball.x(),time1[id].y() - ball.y()));
    }
    //Repulsão da bola no retorno do zagueiro
    if(k==1 && dist_2 < 0.2)
    {
        (*resultante)[0]+=-gain*sin(M_PI/2);
        (*resultante)[1]+=-gain*cos(M_PI/2);
    }

    double min_dist = 999;

    for(int i = 0;i < 3;i++)
    {
        angle = atan2(time2[i].x() - ball.x(),time2[i].y() - ball.y());
        dist = sqrt(pow(ball.x() - time2[i].x(),2.0)+pow( ball.y() - time2[i].y(),2.0));

        if(dist<0.15 && dist > 0.1)
            gain = 4*(0.15 - dist);
        else if(dist < 0.1)
            gain = 0.2;
        else
            gain = 0.0;

        if(min_dist>dist)
            min_dist = dist;

        componenteX = time1[id].x()-gain*sin(angle);
        componenteY = time1[id].y()-gain*cos(angle);

        //Usando o vetor Adversário-bola com um ganho "gain", para composição
        (*resultante)[0]+=-gain*sin(angle);
        (*resultante)[1]+=-gain*cos(angle);

    }


    if(((k==0 && dist_2 < 0.15)||((k==0 && dist_2/min_dist < 1.3)&&time1[id].x()>0)||((k==0 && dist_2/min_dist < 0.6)&&time1[id].x()<=0))
            &&!(time1[id].x()>0.5 && (time1[id].y()<0.35 && time1[id].y()>-0.35)))
    {
        vaiPara_desviando(time1[id],ball.x(),ball.y(),id);
    }else{
        if(time1[id].x()>0.50 && (time1[id].y()<0.35 && time1[id].y()>-0.35))
        {
                vaiPara_desviando(time1[id],0.55,(*resultante)[1],id);
        }
        else
        {
            if(ball.x() > 0.5 && (ball.y()<0.3 && ball.y()>-0.30) && k==1)
                vaiPara_desviando(time1[id],0.4,0.0,id);
            else if(ball.x()<0)
                vaiPara_desviando(time1[id],0.0,(*resultante)[1],id);
            else
                vaiPara_desviando(time1[id],(*resultante)[0],(*resultante)[1],id);
        }


    }
}*/
//}
