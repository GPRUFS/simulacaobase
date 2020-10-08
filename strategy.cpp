#include "strategy.h"

Strategy::Strategy()
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
    int N = 10;
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

    vector <double> destino = {ball.x(),ball.y()};


    //vaiPara2(b0,predictedBall.x,predictedBall.y,0);
    double Xbola;
    double Ybola;

    if (distancia(b2, ball.x(), ball.y()) > 0.5){

        Xbola = predictedBall.x;
        Ybola = predictedBall.y;

    }else{

        Xbola = ball.x();
        Ybola = ball.y();
    }

    goleiro(b0,ball.x(), ball.y(),0);

    zagueiro2(b1,ball.x(), ball.y(),1);

    if(7 == 7){
       vaiPara_hotwheels(b0, b1, b2, y0, y1, y2, Xbola,Ybola,2);
    }

    cinematica_azul();

}

void Strategy::strategy_blue(Team blue, Team yellow, fira_message::Ball ball, const fira_message::Field &field)
{
    vector <double> destino = {ball.x(),ball.y()};
    predict_ball(ball);
    predict_vector = direcao_provavel(predictedBall,ball);

    double Xbola;
    double Ybola;

    if (distancia(blue[2], ball.x(), ball.y()) > 0.5){

        Xbola = predictedBall.x;
        Ybola = predictedBall.y;

    }else{

        Xbola = ball.x();
        Ybola = ball.y();
    }

    goleiro(blue[0],ball.x(), ball.y(),0);

    zagueiro_cone(blue,yellow, ball,1);

    if(7 == 7){
       vaiPara_hotwheels(blue[0], blue[1], blue[2], yellow[0], yellow[1], yellow[2], Xbola,Ybola,2);
    }

    //Obstáculos
    /*vector<State> obs;
    for(int i = 0;i<3;i++)
    {
        if(index_rrt!=i)
            obs.push_back(State(blue[i].x(),blue[i].y()));

        obs.push_back(State(yellow[i].x(),yellow[i].y()));
    }

    RRT(blue[index_rrt],destino,obs);

    State new_ = *(rrt->GetOptimaNodes().end()-1);
     vaiPara(blue[index_rrt],new_.x,new_.y,0);*/


    cinematica_azul();

}

void Strategy::strategy_yellow(fira_message::Robot y0, fira_message::Robot y1,
                     fira_message::Robot y2, fira_message::Ball ball, const fira_message::Field & field)
{
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

    double Kp_l = 5;
    double Ki_l = 1;
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

    if (dist < 0.3){
        dist = 0.3;
    }

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
double Strategy::distancia(State a, State b)
{
    double dist = sqrt( pow((a.x-b.x),2) + pow((a.y-b.y),2) );
    return(dist);
}

double Strategy::limita_velocidade(double valor, double sat)
{
      if (valor > sat) {valor = sat;}
      if (valor < -sat) {valor = -sat;}
      return(valor);
}

void Strategy::RRT(State init, State goal, vector<State> obs_centers, bool flag)
{

    double raio = 0.4;

    double angle = atan2(init.x - goal.x,init.y - goal.y);

    double dist = sqrt(pow(goal.x-init.x,2.0)+pow(goal.y-init.y,2.0));
    State GoalFrontier;
    if(dist < raio)
         GoalFrontier = goal;
    else
    {
        GoalFrontier = State(init.x -  raio*sin(angle),init.y -  raio*cos(angle));
    }


    delete rrt;
    rrt = new rrt_graph(init,GoalFrontier);

    //Loop
    int k = 1;
    for (k = 1;k<400;k++)
    {
        //cout << k << endl;
       // cout << rrt->GetNumNodes() << endl;

        //Sorteia um Ponto aleatório
        State x_rand;
        if(!chave)
            x_rand = rrt->random_state(init,GoalFrontier,0.7,raio);
        else
            x_rand = rrt->random_state(init,GoalFrontier,0.5,0.4,raio,waypopints);

        rrt->Extend_SH(x_rand,obs_centers,0.07,raio);

        if(distancia(rrt->x_new, GoalFrontier)<0.02)
        {
            //Essa etapa pode ser retirada, mas parace ajudar contra dos deadLocks
            rrt->x_new = GoalFrontier;
            //Adiciona vértice à arvore
            rrt->add_vertice(rrt->x_new);
            //Adiciona Aresta
            rrt->add_aresta((rrt->GetNumNodes()-2),(rrt->GetNumNodes()-1));
        }

        if(rrt->x_new == GoalFrontier)
            break;
    }
    rrt->smooth_path(obs_centers,0.07);
    chave = true;
    delete waypopints;
    waypopints = new vector<State>();
    for(int i = rrt->GetNumNodes(); i > (rrt->GetNumNodes()/2);i--)
        waypopints->push_back(rrt->GetNodeState(i));

}

void Strategy::RRT(fira_message::Robot rb, vector<double> _goal, vector<State> obs_centers, bool flag)
{
    double raio = 0.4;

    State init = State(rb.x(),rb.y());
    State goal = State(_goal[0],_goal[1]);


    double angle = atan2(init.x - goal.x,init.y - goal.y);

    double dist = sqrt(pow(goal.x-init.x,2.0)+pow(goal.y-init.y,2.0));
    State GoalFrontier;
    if(dist < raio)
         GoalFrontier = goal;
    else
    {
        GoalFrontier = State(init.x -  raio*sin(angle),init.y -  raio*cos(angle));
    }


    delete rrt;
    rrt = new rrt_graph(init,GoalFrontier);

    //Loop
    int k = 1;
    for (k = 1;k<400;k++)
    {
        //cout << k << endl;
       // cout << rrt->GetNumNodes() << endl;

        //Sorteia um Ponto aleatório
        State x_rand;
        if(!chave)
            x_rand = rrt->random_state(init,GoalFrontier,0.7,raio);
        else
            x_rand = rrt->random_state(init,GoalFrontier,0.5,0.4,raio,waypopints);

        rrt->Extend_SH(x_rand,obs_centers,0.07,raio);

        if(distancia(rrt->x_new, GoalFrontier)<0.02)
        {
            //Essa etapa pode ser retirada, mas parace ajudar contra dos deadLocks
            rrt->x_new = GoalFrontier;
            //Adiciona vértice à arvore
            rrt->add_vertice(rrt->x_new);
            //Adiciona Aresta
            rrt->add_aresta((rrt->GetNumNodes()-2),(rrt->GetNumNodes()-1));
        }

        if(rrt->x_new == GoalFrontier)
            break;
    }
    rrt->smooth_path(obs_centers,0.07);
    chave = true;
    delete waypopints;
    waypopints = new vector<State>();
    for(int i = rrt->GetNumNodes(); i > (rrt->GetNumNodes()/2);i--)
        waypopints->push_back(rrt->GetNodeState(i));

}

Strategy::~Strategy()
{

}

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

/*void Strategy::goleiro_petersson(fira_message::Robot rb,fira_message::Ball ball, int id){

    double gol_top = 0.2;
    double campo_y = 0.7;
    double gol_x = 0.7;
    double lim_x = 0.03;
    double lim_ang = 2;

    vector <double> new_pos = {0,0};

    double dist = distancia(rb,ball.x(),ball.y());

    //se a bola estiver l
    if ( dist > 0.3){
        new_pos = {predictedBall.x,predictedBall.y};
    }else{
        new_pos = {ball.x(),ball.y()};
    }

    vector<double> destino ={-gol_x,0};

    //envia goleiro para o meio do gol antes de tudo
    if (rb.x() > -gol_x + lim_x || rb.x() < -gol_x - lim_x){
        vaiPara_desviando(rb,destino[0],destino[1],id);
        printf("indo centro\n");
    }else{
        //corrige a orientação do robô para olhar para o topo do campo
        ang_err angulo = olhar(rb,rb.x(),campo_y);
        printf("olhando\n");
        if (angulo.fi > lim_ang || angulo.fi <-lim_ang){
            //VW[id][1] = irponto_angular(rb,rb.x(),campo_y);
        }else{
            printf("seguindo bola\n");
            //limita os valores superiores e inferiores que o goleiro pode ir
            if(new_pos[1] > gol_top){
                new_pos[1] = gol_top;
            }

            if(new_pos[1] < -gol_top){
                new_pos[1] = -gol_top;
            }
            //se a bola estiver acima faz o robô subir, se estiver abaixo faz ele descer
            if(new_pos[1] > rb.y()){
               andarFundo(125,id);
            }

            if(new_pos[1] < rb.y()){
               andarFrente(125,id);
            }

        }
    }
}
*/
void Strategy::vaiPara_desviando(fira_message::Robot rb,double px,double py,int id){

    double V[2] = {px - rb.x(),py - rb.y()};

    double F[2] = {0,0};

    calc_repulsao(rb,F);
  
    double kr = 1.0;
    double ka = 1.0;


    double new_pos[2] = {rb.x() + ka*V[0] + kr*F[0],rb.y() + ka*V[1] + kr*F[1]};

    Strategy::saturacao(new_pos);

    vaiParaDinamico(rb,new_pos[0],new_pos[1],id);

    filtro(VW[id][0],id);
}


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

// Vetor que usa a predição e estado atual para gerar um vetor de direção provável da bola
vector<double> Strategy::direcao_provavel(ballPredPos pred, fira_message::Ball ball)
{
    vector<double> vec = {pred.x - ball.x(), pred.y - ball.y()};
    return vec;
}

//opção = true -> vertical
//opção = false -> horizontal
void Strategy::posicionamento(fira_message::Robot rb, int id, bool opcao)
{
    double limit = 10;
    ang_err angulo;
    if(opcao)
         angulo = olhar(rb,rb.x(),limit + 5); // calcula diferença entre angulo atual e angulo desejado
    else
        angulo = olhar(rb,limit,rb.y()); // calcula diferença entre angulo atual e angulo desejado

    printf("%i\n", angulo.flag);
    if(angulo.fi >= 0.5 || angulo.fi<= -0.5){ //se o robô não está aproximadamente 90 graus
        andarFrente(0,id);
        VW[id][1] = controleAngular(angulo.fi);

    }

}


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

//Goleiro de Petersson
void Strategy::goleiro2(fira_message::Robot rb,fira_message::Ball ball, int id){

    double gol_top = 0.35;
    double campo_y = 0.7;
    double gol_x = 0.7;
    double lim_x = 0.02;
    double lim_ang = 2;

    //envia goleiro para o meio do gol antes de tudo
    if (rb.x() > -gol_x + lim_x || rb.x() < -gol_x - lim_x){
       vaiParaDinamico(rb,-gol_x,rb.y(),id);
       printf("ajustando posição\n");
    }else{
        ang_err angulo = olhar(rb,rb.x(),gol_top);
        if (angulo.fi > lim_ang || angulo.fi <-lim_ang){
            //VW[id][1] = irponto_angular(rb,rb.x(),campo_y);
            printf("ajustando angulo\n");
        }else{
            if (ball.y() < gol_top && ball.y() >-gol_top){
                 if (rb.y() > ball.y()){
                     andarFrente(100,id);
                     printf("seguindo bola\n");
                 }else{
                     andarFundo(100,id);
                     printf("seguindo bola\n");
                 }
            }
        }
    }
}

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
      if(distancia(rb,xbola,ybola) < 0.2){ //robô proóximo da bola
          chute(id);
      }
  }

}

void Strategy::chute(int id){
    VW[id][1] = -100;
}
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

        VW[id][0] = filtro(VW[id][0],id);

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

                VW[id][1] = filtro(VW[id][1],id);


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

                VW[id][2] = filtro(VW[id][2],id);



            }
        }
}
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

//Zagueiro de cone com a ideia de predição potencial de antecipação usando uma espécie de "Motor Schema"

void Strategy::zagueiro_cone(Team blue, Team yellow, fira_message::Ball ball, int id)
{
    double raio = 0.08;
    double angle;
    double dist;
    double gain = 3*raio;  //Ganho inicial, dependência da região do campo
    double v_of;
    if(ball.x()-blue[id].x()<0)
        v_of = ball.x()-blue[id].x();
    else
        v_of = 0;

    double dist_2 = sqrt(pow(ball.x()-blue[id].x(),2.0)+pow( ball.y()-blue[id].y(),2.0));


    angle = atan2(blue[id].x() - predictedBall.x - v_of,blue[id].y() - predictedBall.y);
    double componenteX = blue[id].x() -  gain*sin(angle);
    double componenteY = blue[id].y() -  gain*cos(angle);


    for(int i = 0;i<3;i++)
    {
        angle = atan2(yellow[i].x() - ball.x(),yellow[i].y() - ball.y());
        dist = sqrt(pow(ball.x()-yellow[i].x(),2.0)+pow( ball.y()-yellow[i].y(),2.0));
        gain = raio/dist;
        if(gain>5*raio)
            gain = 5*raio;

        componenteX += -gain*sin(angle);
        componenteY += -gain*sin(angle);
    }

    vaiPara(blue[id],componenteX,componenteY,id);

    if(v_of==0 || dist_2 < 0.15)
        vaiPara(blue[id],ball.x(),ball.y(),id);




}
// Zagueiro David + Cone
void Strategy::zagueiro2(fira_message::Robot rb, double xbola, double ybola, int id){
    double x_penalti =  0.4;
    double x_meio_de_campo = 0.0;
    double x_radius = 0.2;
    double y_top = 0.65;
    double ala_deepth = 0.3;
    double K_press = 0.2;

    double media_x = 0;
    double media_y = 0;


    if(xbola > x_penalti)
    {    //Se a Bola estiver na zona "A"
         media_x = ((x_meio_de_campo + K_press + 0.1) + rb.x() )/2;
         media_y = (ybola + rb.y() )/2;
         vaiPara(rb,media_x,media_y,id);

        printf("AAA !! \n");
    }
    else if(xbola >= x_meio_de_campo
            && (ybola < (y_top - ala_deepth)
               && ybola > (ala_deepth - y_top)))
    {    //Se a Bola estiver na zona "B_mid"
        media_x = (-x_penalti + rb.x() )/2;
        media_y = (ybola + rb.y() )/2;
        vaiPara(rb,media_x,media_y,id);
        printf("BBB mid !! \n");
    }
    else if((xbola >= x_meio_de_campo)
             && (ybola > (y_top - ala_deepth) || ybola < (ala_deepth - y_top)))
    {    //Se a Bola estiver na zona "B_top" ou "B_bot"
        media_x = (-x_radius + rb.x() )/2;
        media_y = (ybola + rb.y() )/2;
        vaiPara(rb,media_x,media_y,id);
        printf("BBB top ou bot !! \n");
    }
    else if(xbola < x_meio_de_campo
            && xbola > rb.x()
              && (ybola < (y_top - ala_deepth)
                && ybola > (ala_deepth - y_top)))
    {    //Se a Bola estiver na zona "C"
        media_x = (xbola + rb.x() )/2;
        media_y = (ybola + rb.y() )/2;
        vaiPara(rb,media_x,media_y,id);
        printf("CCC !! \n");
    }
    else if((xbola < x_meio_de_campo
             && ybola > (y_top - ala_deepth)))
    {    //Se a Bola estiver na zona "D"
        media_x = (xbola + rb.x() )/2;
        media_y = ((y_top - ala_deepth) + rb.y() )/2;
        vaiPara(rb,media_x,media_y,id);
        printf("DDD !! \n");
    }
    else if((xbola < x_meio_de_campo)
             && ybola < (ala_deepth - y_top))
    {    //Se a Bola estiver na zona "E"
        media_x = (xbola + rb.x() )/2;
        media_y = ((ala_deepth - y_top) + rb.y() )/2;

        vaiPara(rb,media_x,media_y,id);
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

