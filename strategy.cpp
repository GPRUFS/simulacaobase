#include "strategy.h"

Strategy::Strategy()
{
    L = 0.04; //Distância entre roda e centro
    R = 0.02; //Raio da roda

    vector<double> aux{0,0};

    qtdRobos = 3;
    vrMax = 125;

    Vmax = 2.5;
    Wmax = 62.5;

    for(int i = 0; i < qtdRobos; i++)
    {
        vRL.push_back(aux);
        VW.push_back(aux);
    }
}

double Strategy::irponto_linear(fira_message::Robot robot, double x, double y)
{
    double err_x = x-robot.x();
    double err_y = y-robot.y();
    double dtheta = 0;
    double dp = 0;
    double V = 0;

    dp = sqrt(pow(err_x,2)+pow(err_y,2));

    V = Vmax*tanh(0.3*dp);
    return V;
}

double Strategy::irponto_angular(fira_message::Robot robot, double x, double y)
{
    //Precisa saber se olha de frente ou de costas
    //Ângulos são em radianos
    double err_x = x-robot.x();
    double err_y = y-robot.y();
    double theta = 0;
    double dtheta_frente = 0;
    double dtheta_costas = 0;
    double dtheta = 0;
    double W = 0;

    theta = atan2(err_y,err_x); //Ângulo desejado

    dtheta_frente = theta-robot.orientation();
    dtheta_costas = theta-(robot.orientation()+M_PI);
    dtheta = (dtheta_frente<dtheta_costas)?(dtheta_frente):(dtheta_costas); //O menor é o executado, não tá muito certo

    W = Wmax*tanh(0.03*dtheta);
    return W;
}

void Strategy::strategy_blue(fira_message::Robot b0, fira_message::Robot b1,
                   fira_message::Robot b2, fira_message::Ball ball, const fira_message::Field & field)
{

    //V0_b = irponto_linear(b0,ball.x(),ball.y());

    ang_err angulo = olhar(b0, ball.x(), ball.y());

    //VW[0][1] = controleAngular(angulo.fi);
    vaiPara(b0,ball.x(),ball.y(),0);

    printf("Orientacao:%f\n",b0.orientation()*180/M_PI);
    printf("Angulo:%f\n",angulo.fi);
    printf("V:%f\n",VW[0][0]);
    //printf("W:%f\n",VW[0][1]);

    andarFrente(100,1);
    girarHorario(50,2);

    //No final todas as velocidades devem estar definidas e apenas a última definição será considerada
    //Convertendo as velocidades
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

void Strategy::cinematica_amarelo()
{
    //TODO
}


void Strategy::andarFrente(double vel, int id)
{
    double Vaux = vel/vrMax;
    VW[id][0] = Vmax*Vaux;
    VW[id][1] = 0;
}

void Strategy::andarFundo(double vel, int id)
{
    double Vaux = vel/vrMax;
    VW[id][0] = -Vmax*Vaux;
    VW[id][1] = 0;
}

void Strategy::vaiPara(fira_message::Robot rb, double px, double py, int id)
{
    VW[id][0] = controleLinear(rb,px,py);
    ang_err angulo = olhar(rb, px, py);
    VW[id][1] = controleAngular(angulo.fi);
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

    return(Waux); //deve tetornar um valor entre -1 e 1
}

double Strategy::controleLinear(fira_message::Robot rb,double px, double py)
{
    double  Vaux = 0;
    double  k_lin = 2;   //constante de contração da tangente hiperbólica
    double  V_max = Vmax;       //constante limitante da tangente hiperbólica
    double  v_min = 0.03;  	 //módulo da velocidade linear mínima permitida
    double  ang_grande = 30; //para ângulos maiores que esse valor o sistema da prioridade ao W, reduzindo o V
    double  dist = distancia(rb, px, py);

    ang_err angulo = olhar(rb, px, py);

    Vaux = V_max*tanh(k_lin*dist*angulo.flag);  //controle não linear de V

    if (Vaux*angulo.flag < v_min) Vaux = v_min*angulo.flag;  //aplica o valor definido em v_min

                                                       //if (angulo.fi*angulo.flag > ang_grande) V = v_min*angulo.flag;  // controle de prioridade reduzindo V quando "ang_err" for grande
    Vaux = Vaux*cos(angulo.fi*M_PI / 180);// controle de prioridade reduzindo V quando "ang_err" for grande

    Vaux = limita_velocidade(Vaux, Vmax); //satura em -1 a 1

    return(Vaux);
}

double Strategy::limita_velocidade(double valor, double sat)
{
      if (valor > sat) {valor = sat;}
      if (valor < -sat) {valor = -sat;}
      return(valor);
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

Strategy::~Strategy()
{

}