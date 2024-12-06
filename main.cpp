//
//  main.cpp
//  paint
//
//  Created by Gabriel Lopes Bastos on 30/10/24.
//


// Bibliotecas utilizadas pelo OpenGL
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <forward_list>
#include <algorithm> // Para swap
#include "glut_text.h"
#include <vector>
#include <queue>

using namespace std;

// Variaveis Globais
#define ESC 27
#define ENTER 13
int preto[3] {0, 0, 0};

//Enumeracao com os tipos de formas geometricas
enum tipo_forma{LIN = 1, TRI, QUAD, POL, TRAN, CIR }; // Linha, Triangulo, Quadrilatero, Circulo

//Verifica se foi realizado o primeiro clique do mouse
bool click1 = false;

//Coordenadas da posicao atual do mouse
int m_x, m_y;

//Coordenadas do primeiro clique e do segundo clique do mouse
int x_1, y_1, x_2, y_2;

//Indica o tipo de forma geometrica ativa para desenhar
int modo = LIN;

//Largura e altura da janela
int width = 512, height = 512;

// Definicao de vertice
struct vertice{
    int x;
    int y;
    int cor[3] = {0, 0, 0};
};

// Lista de vértices para os polígonos global
static forward_list<vertice> poligonoVertices;
// Lista de vertices pintados no flood fill
static forward_list<vertice> floodFillVertices;

// Definicao das formas geometricas
struct forma{
    int tipo;
    forward_list<vertice> v; //lista encadeada de vertices
    forward_list<vertice> preenchidos; //lista encadeada de pixels preenchidos
    bool preenchido = false;
    int cor[3] = {0, 0, 0};
};

// Lista encadeada de formas geometricas
forward_list<forma> formas;

// Funcao para armazenar uma forma geometrica na lista de formas
// Armazena sempre no inicio da lista
void pushForma(int tipo){
    forma f;
    f.tipo = tipo;
    formas.push_front(f);
}

// Funcao para armazenar um vertice na forma do inicio da lista de formas geometricas
// Armazena sempre no inicio da lista
void pushVertice(int x, int y){
    vertice v;
    v.x = x;
    v.y = y;
    formas.front().v.push_front(v);
}

//Fucao para armazenar uma Linha na lista de formas geometricas
void pushLinha(int x1, int y1, int x2, int y2){
    pushForma(LIN);
    pushVertice(x1, y1);
    pushVertice(x2, y2);
}

void pushQuad(int x1, int y1, int x2, int y2) {
    pushForma(QUAD);
    pushVertice(x1, y1);
    pushVertice(x2, y1);
    pushVertice(x2, y2);
    pushVertice(x1, y2);
}

void pushTri(int x1, int y1, int x2, int y2, int x3, int y3) {
    pushForma(TRI);
    pushVertice(x1, y1);
    pushVertice(x2, y2);
    pushVertice(x3, y3);
}

void pushCirculo(int x, int y, int raio) {
    pushForma(CIR);
    pushVertice(x, y);
    pushVertice(raio, raio);
}

void preencherPoligono(forma& f);

// Função para calcular o centroide de uma forma
vertice calcularCentroide(const forma& f) {
    int cx = 0, cy = 0, n = 0;
    for (const auto& vert : f.v) {
        cx += vert.x;
        cy += vert.y;
        ++n;
    }
    if (n > 0) {
        cx /= n;
        cy /= n;
    }
    return {cx, cy};
}

// Função para multiplicar um vértice por uma matriz de transformação
vertice aplicarTransformacao(const vertice& v, const std::array<std::array<float, 3>, 3>& matriz) {
    vertice resultado;
    resultado.x = static_cast<int>(v.x * matriz[0][0] + v.y * matriz[0][1] + matriz[0][2]);
    resultado.y = static_cast<int>(v.x * matriz[1][0] + v.y * matriz[1][1] + matriz[1][2]);
    return resultado;
}

// Função para criar uma matriz de translação
std::array<std::array<float, 3>, 3> criarMatrizTranslacao(float dx, float dy) {
    return {{
        {1, 0, dx},
        {0, 1, dy},
        {0, 0, 1}
    }};
}

// Função para criar uma matriz de escala
std::array<std::array<float, 3>, 3> criarMatrizEscala(float fator) {
    return {{
        {fator, 0, 0},
        {0, fator, 0},
        {0, 0, 1}
    }};
}

// Função para criar uma matriz de rotação
std::array<std::array<float, 3>, 3> criarMatrizRotacao(float angulo) {
    float rad = angulo * M_PI / 180.0;
    return {{
        {cos(rad), -sin(rad), 0},
        {sin(rad), cos(rad), 0},
        {0, 0, 1}
    }};
}

// Função para multiplicar duas matrizes 3x3
std::array<std::array<float, 3>, 3> multiplicarMatrizes(const std::array<std::array<float, 3>, 3>& A, const std::array<std::array<float, 3>, 3>& B) {
    std::array<std::array<float, 3>, 3> resultado = {0};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                resultado[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    return resultado;
}

// Função para aplicar uma transformação composta a uma forma
void aplicarTransformacaoComposta(forma& f, const std::array<std::array<float, 3>, 3>& matriz) {
    for (auto& vert : f.v) {
        vert = aplicarTransformacao(vert, matriz);
    }
}

// Função para escalar uma forma
void escalarForma(forma& f, float fator) {
    // Calcular o centroide da forma
    int cx = 0, cy = 0, n = 0;
    for (const auto& vert : f.v) {
        cx += vert.x;
        cy += vert.y;
        ++n;
    }
    cx /= n;
    cy /= n;

    // Criar a matriz de transformação composta
    auto matrizTranslacaoOrigem = criarMatrizTranslacao(-cx, -cy);
    auto matrizEscala = criarMatrizEscala(fator);
    auto matrizTranslacaoDeVolta = criarMatrizTranslacao(cx, cy);
    auto matrizComposta = multiplicarMatrizes(multiplicarMatrizes(matrizTranslacaoDeVolta, matrizEscala), matrizTranslacaoOrigem);

    // Aplicar a transformação composta aos vértices
    aplicarTransformacaoComposta(f, matrizComposta);

    // limpa os pixels preenchidos e preenche novamente
    if (f.preenchido) {
        f.preenchidos.clear();
        preencherPoligono(f);
    }
    
}

// Função para rotacionar uma forma
void rotacionarForma(forma& f, float angulo) {
    // Calcular o centroide da forma
    int cx = 0, cy = 0, n = 0;
    for (const auto& vert : f.v) {
        cx += vert.x;
        cy += vert.y;
        ++n;
    }
    cx /= n;
    cy /= n;

    // Criar a matriz de transformação composta
    auto matrizTranslacaoOrigem = criarMatrizTranslacao(-cx, -cy);
    auto matrizRotacao = criarMatrizRotacao(angulo);
    auto matrizTranslacaoDeVolta = criarMatrizTranslacao(cx, cy);
    auto matrizComposta = multiplicarMatrizes(multiplicarMatrizes(matrizTranslacaoDeVolta, matrizRotacao), matrizTranslacaoOrigem);

    // Aplicar a transformação composta
    aplicarTransformacaoComposta(f, matrizComposta);

    // limpa os pixels preenchidos e preenche novamente
    if (f.preenchido) {
        f.preenchidos.clear();
        preencherPoligono(f);
    }

}

// Função para transladar uma forma
void transladarForma(forma& f, float dx, float dy) {
    auto matrizTranslacao = criarMatrizTranslacao(dx, dy);
    aplicarTransformacaoComposta(f, matrizTranslacao);

    // Aplicar a transformação composta aos pixels preenchidos
    for (auto& p : f.preenchidos) {
        p = aplicarTransformacao(p, matrizTranslacao);
    }
}

// Função para cisalhar uma forma em torno do centroide
void cisalharForma(forma& f, float dx, float dy) {
    // Calcular o centroide da forma
    int cx = 0, cy = 0, n = 0;
    for (const auto& vert : f.v) {
        cx += vert.x;
        cy += vert.y;
        ++n;
    }
    cx /= n;
    cy /= n;

    // Criar a matriz de transformação composta
    auto matrizTranslacaoOrigem = criarMatrizTranslacao(-cx, -cy);
    auto matrizCisalhamento = std::array<std::array<float, 3>, 3> {{
        {1, dx, 0},
        {dy, 1, 0},
        {0, 0, 1}
    }};
    auto matrizTranslacaoDeVolta = criarMatrizTranslacao(cx, cy);
    auto matrizComposta = multiplicarMatrizes(multiplicarMatrizes(matrizTranslacaoDeVolta, matrizCisalhamento), matrizTranslacaoOrigem);

    // Aplicar a transformação composta
    aplicarTransformacaoComposta(f, matrizComposta);

    // limpa os pixels preenchidos e preenche novamente
    if (f.preenchido) {
        f.preenchidos.clear();
        preencherPoligono(f);
    }
}

// Função para refletir uma forma
void refletirForma(forma& f, bool vertical, bool horizontal) {
    // Calcular o centroide da forma
    int cx = 0, cy = 0, n = 0;
    for (const auto& vert : f.v) {
        cx += vert.x;
        cy += vert.y;
        ++n;
    }
    cx /= n;
    cy /= n;

    // Criar a matriz de transformação composta
    auto matrizTranslacaoOrigem = criarMatrizTranslacao(-cx, -cy);
    auto matrizReflexao = std::array<std::array<float, 3>, 3> {{
        {horizontal ? -1.0f : 1.0f, 0.0f, 0.0f},
        {0.0f, vertical ? -1.0f : 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    }};
    auto matrizTranslacaoDeVolta = criarMatrizTranslacao(cx, cy);
    auto matrizComposta = multiplicarMatrizes(multiplicarMatrizes(matrizTranslacaoDeVolta, matrizReflexao), matrizTranslacaoOrigem);

    // Aplicar a transformação composta
    aplicarTransformacaoComposta(f, matrizComposta);

    // Aplicar a transformação composta aos pixels preenchidos
    for (auto& p : f.preenchidos) {
        p = aplicarTransformacao(p, matrizComposta);
    }
}



/*
 * Declaracoes antecipadas (forward) das funcoes (assinaturas das funcoes)
 */
void init(void);
void reshape(int w, int h);
void display(void);
void menu_popup(int value);
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void mousePassiveMotion(int x, int y);
void drawPixel(int x, int y, int cor[3]);
// Funcao que percorre a lista de formas geometricas, desenhando-as na tela
void drawFormas();
void retaBresenhan(int x1, int y1, int x2, int y2);
void circuloBresenhan(int x, int y, int raio);
void preencherTodasFormas();



/*
 * Funcao principal
 */
int main(int argc, char** argv){
    glutInit(&argc, argv); // Passagens de parametro C para o glut
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB); //Selecao do Modo do Display e do Sistema de cor
    glutInitWindowSize (width, height);  // Tamanho da janela do OpenGL
    glutInitWindowPosition (100, 100); //Posicao inicial da janela do OpenGL
    glutCreateWindow ("Computacao Grafica: Paint"); // Da nome para uma janela OpenGL
    init(); // Chama funcao init();
    glutReshapeFunc(reshape); //funcao callback para redesenhar a tela
    glutKeyboardFunc(keyboard); //funcao callback do teclado
    glutMouseFunc(mouse); //funcao callback do mouse
    glutPassiveMotionFunc(mousePassiveMotion); //fucao callback do movimento passivo do mouse
    glutDisplayFunc(display); //funcao callback de desenho
    
    // Criação do Menu
    glutCreateMenu(menu_popup);
    glutAddMenuEntry("Linha", LIN);
    glutAddMenuEntry("Quadrilatero", QUAD);
    glutAddMenuEntry("Triangulo", TRI);
    glutAddMenuEntry("Poligono", POL);
    glutAddMenuEntry("Circulo", CIR);
    glutAddMenuEntry("Sair", 0);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    
    glutMainLoop(); // executa o loop do OpenGL
    return EXIT_SUCCESS; // retorna 0 para o tipo inteiro da funcao main();
}

/*
 * Inicializa alguns parametros do GLUT
 */
void init(void){
    glClearColor(1.0, 1.0, 1.0, 1.0); //Limpa a tela com a cor branca;
}

/*
 * Ajusta a projecao para o redesenho da janela
 */
void reshape(int w, int h)
{
    // Muda para o modo de projecao e reinicializa o sistema de coordenadas
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Definindo o Viewport para o tamanho da janela
    glViewport(0, 0, w, h);
    
    width = w;
    height = h;
    glOrtho (0, w, 0, h, -1 ,1);

   // muda para o modo de desenho
    glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();

}

/*
 * Controla os desenhos na tela
 */
void display(void){
    glClear(GL_COLOR_BUFFER_BIT); //Limpa o buffer de cores e reinicia a matriz
    glColor3f (0.0, 0.0, 0.0); // Seleciona a cor default como preto
    drawFormas(); // Desenha as formas geometricas da lista
    //Desenha texto com as coordenadas da posicao do mouse
    draw_text_stroke(0, 0, "(" + to_string(m_x) + "," + to_string(m_y) + ")", 0.2);
    glutSwapBuffers(); // manda o OpenGl renderizar as primitivas
}

/*
 * Controla o menu pop-up
 */
void menu_popup(int value){
    if (value == 0) exit(EXIT_SUCCESS);
    modo = value;
}


/*
 * Controle das teclas comuns do teclado
 */
void keyboard(unsigned char key, int x, int y){
    switch (key) { // key - variavel que possui valor ASCII da tecla precionada
        case ESC: exit(EXIT_SUCCESS); break;
        case ENTER: {
            if (modo == POL) {
                if (std::distance(poligonoVertices.begin(), poligonoVertices.end()) >= 4) {
                    pushForma(POL);
                    for (const auto& vert : poligonoVertices) {
                        pushVertice(vert.x, vert.y);
                    }
                    poligonoVertices.clear();
                    glutPostRedisplay();
                }
            }
            break;
        }
        // Função tecle "p" para preencher o último polígono
        case 'p': {
            if (!formas.empty() && formas.front().tipo != CIR && formas.front().tipo != LIN) {
                preencherPoligono(formas.front());
                glutPostRedisplay();
            }
            break;
        }
        // Função tecle "f" para preencher todas as formas não preenchidas com o flood fill
        case 'f': {
            if (!formas.empty()) {
                preencherTodasFormas();
                glutPostRedisplay();
            }
            break;
        }
    }
    // Controle de teclas para as transformações geométricas
    if (!formas.empty() && formas.front().tipo != CIR) {
        bool redisplay = true;
        switch (key) {
            case 'w': {
                transladarForma(formas.front(), 0, 10);
                break;
            }
            case 's': {
                transladarForma(formas.front(), 0, -10);
                break;
            }
            case 'a': {
                transladarForma(formas.front(), -10, 0);
                break;
            }
            case 'd': {
                transladarForma(formas.front(), 10, 0);
                break;
            }
            case 'E': {
                escalarForma(formas.front(), 0.9);
                break;
            }
            case 'e': {
                escalarForma(formas.front(), 1.1);
                break;
            }
            case 'r': {
                rotacionarForma(formas.front(), -10);
                break;
            }
            case 'R': {
                rotacionarForma(formas.front(), 10);
                break;
            }
            case 'c': {
                cisalharForma(formas.front(), 0.1, 0);
                break;
            }
            case 'C': {
                cisalharForma(formas.front(), -0.1, 0);
                break;
            }
            case 'y': {
                cisalharForma(formas.front(), 0, 0.1);
                break;
            }
            case 'Y': {
                cisalharForma(formas.front(), 0, -0.1);
                break;
            }
            case 'v': {
                refletirForma(formas.front(), true, false);
                break;
            }
            case 'h': {
                refletirForma(formas.front(), false, true);
                break;
            }
            default: {
                redisplay = false;
                break;
            }
        }
        if (redisplay) {
            glutPostRedisplay();
        }
    }
}

/*
 * Controle dos botoes do mouse
 */
void mouse(int button, int state, int x, int y){
    switch (button) {
        case GLUT_LEFT_BUTTON:
            switch(modo){
                // Controle do mouse para linha
                case LIN:
                    if (state == GLUT_DOWN) {
                        if(click1){
                            x_2 = x;
                            y_2 = height - y - 1;
                            pushLinha(x_1, y_1, x_2, y_2);
                            click1 = false;
                            glutPostRedisplay();
                        }else{
                            click1 = true;
                            x_1 = x;
                            y_1 = height - y - 1;
                        }
                    }
                break;
                // Controle do mouse para quadrilatero com 2 pontos
                case QUAD:
                    if (state == GLUT_DOWN) {
                        if (click1) {
                            x_2 = x;
                            y_2 = height - y - 1;
                            pushQuad(x_1, y_1, x_2, y_2);
                            click1 = false;
                            glutPostRedisplay();
                        }else{
                            click1 = true;
                            x_1 = x;
                            y_1 = height - y - 1;
                        }
                    }
                break;
                case TRI:
                    if (state == GLUT_DOWN) {
                        static int clickCount = 0;
                        if (clickCount == 0) {
                            x_1 = x;
                            y_1 = height - y - 1;
                            clickCount++;
                        } else if (clickCount == 1) {
                            x_2 = x;
                            y_2 = height - y - 1;
                            clickCount++;
                        } else if (clickCount == 2) {
                            int x_3 = x;
                            int y_3 = height - y - 1;
                            pushTri(x_1, y_1, x_2, y_2, x_3, y_3);
                            clickCount = 0;
                            glutPostRedisplay();
                        }
                    }
                break;
                // Controle do mouse para poligono com 4+ vertices
                case POL:
                    if (state == GLUT_DOWN) {
                        int x_click = x;
                        int y_click = height - y - 1;
                        poligonoVertices.push_front({x_click, y_click}); // Adiciona o vértice
                    }
                break;
                // Controle do mouse para circunferencia
                case CIR:
                    if (state == GLUT_DOWN) {
                        if (click1) {
                            x_2 = x;
                            y_2 = height - y - 1;
                            int raio = sqrt(pow(x_2 - x_1, 2) + pow(y_2 - y_1, 2));
                            pushCirculo(x_1, y_1, raio);
                            click1 = false;
                            glutPostRedisplay();
                        } else {
                            click1 = true;
                            x_1 = x;
                            y_1 = height - y - 1;
                        }
                    }
            }
        break;
    }
}

/*
 * Controle da posicao do cursor do mouse
 */
void mousePassiveMotion(int x, int y){
    m_x = x; m_y = height - y - 1;
    glutPostRedisplay();
}

/*
 * Funcao para desenhar apenas um pixel na tela
 */
void drawPixel(int x, int y, int cor[3]){
    glBegin(GL_POINTS);
    glColor3ub(cor[0], cor[1], cor[2]);
    glVertex2i(x, y);
    glEnd();
}

/*
 * Funcao que desenha a lista de formas geometricas
 */
void drawFormas() {
    // Apos o primeiro clique, desenha a reta com a posicao atual do mouse
    if (click1 && modo == LIN) retaBresenhan(x_1, y_1, m_x, m_y);
    
    // Percorre a lista de formas geometricas para desenhar
    for (auto& f : formas) {
        switch (f.tipo) {
            // Desenha linha
            case LIN: {
                int i = 0, x[2], y[2];
                // Percorre a lista de vertices da forma linha para desenhar
                for (auto& v : f.v) {
                    x[i] = v.x;
                    y[i] = v.y;
                    ++i;
                }
                // Desenha o segmento de reta apos dois cliques
                retaBresenhan(x[0], y[0], x[1], y[1]);
                break;
            }
            // Desenha quadrilatero com 2 pontos
            case QUAD: {
                int i = 0, x[4], y[4];
                for (auto& v : f.v) {
                    x[i] = v.x;
                    y[i] = v.y;
                    ++i;
                }
                retaBresenhan(x[0], y[0], x[1], y[1]);
                retaBresenhan(x[1], y[1], x[2], y[2]);
                retaBresenhan(x[2], y[2], x[3], y[3]);
                retaBresenhan(x[3], y[3], x[0], y[0]);
                break;
            }
            // Desenha triangulo 
            case TRI: {
                int i = 0, x[3], y[3];
                for (auto& v : f.v) {
                    x[i] = v.x;
                    y[i] = v.y;
                    ++i;
                }
                retaBresenhan(x[0], y[0], x[1], y[1]);
                retaBresenhan(x[1], y[1], x[2], y[2]);
                retaBresenhan(x[2], y[2], x[0], y[0]);
                break;  
            }
            // Desenha poligono com 4+ vertices
            case POL: {
                int prev_x = -1, prev_y = -1;
                for (auto& v : f.v) {
                    if (prev_x != -1 && prev_y != -1) {
                        retaBresenhan(prev_x, prev_y, v.x, v.y);
                    }
                    prev_x = v.x;
                    prev_y = v.y;
                }
                // Connect the last vertex to the first to close the polygon
                if (!f.v.empty()) {
                    retaBresenhan(prev_x, prev_y, f.v.front().x, f.v.front().y);
                }
                break;
            }
            // Desenha circunferencia
            case CIR: {
                int i = 0, x[2], y[2];
                for (auto& v : f.v) {
                    x[i] = v.x;
                    y[i] = v.y;
                    ++i;
                }
                int raio = x[0];
                circuloBresenhan(x[1], y[1], raio);
                break;
            }
        }

        // Desenha os pontos preenchidos
        int cor[3] = {f.cor[0], f.cor[1], f.cor[2]};
        for (const auto& p : f.preenchidos) {
            drawPixel(p.x, p.y, cor);
        }
    }
}

/*
* Funcao que implementa Algoritmo de Bresenhan na rasterização de uma reta
*/
void retaBresenhan(int x1, int y1, int x2, int y2) {
        int x, y;
        bool declive = false, simetrico = false;
        int deltaX, deltaY;
        deltaX = x2 - x1;
        deltaY = y2 - y1;
        if ((deltaX * deltaY) < 0) {
            y1 *= -1;
            y2 *= -1;
            deltaY *= -1;
            simetrico = true;
        }
        if (abs(deltaX) < abs(deltaY)) {
            swap(x1, y1);
            swap(x2, y2);
            swap(deltaX, deltaY);
            declive = true;
        }
        if (x1 > x2) {
            swap(x1, x2);
            swap(y1, y2);
            deltaX *= -1;
            deltaY *= -1;
        }
        
        int d, incE, incNE;
        d = 2*deltaY - deltaX;
        incE = 2*deltaY;
        incNE = 2*(deltaY - deltaX);
        x = x1; y = y1;
        
        while (x <= x2) {
            int xi = x, yi = y;
            if (declive) swap(xi, yi);
            if (simetrico) yi *= -1;
            if (d <= 0) {
                d += incE;
            } else {
                d += incNE;
                y++;
            }
            x++;
            
            drawPixel(xi, yi, preto);
        }
}

/*
* Função para desenhar um círculo utilizando o algoritmo de Bresenhan
*/
void circuloBresenhan(int x, int y, int raio) {
    // Translada o centro do círculo para a origem
    int cx = x, cy = y, d, incE, incSE;
    x = 0;
    y = raio;

    // Cria um vetor para armazenar os pixels ao redor do centroide e só depois desenha na tela
    std::vector<std::pair<int, int>> pixels;

    d = 1 - raio;
    incE = 3;
    incSE = -2 * raio + 5;

    while (x <= y) {
        pixels.push_back({x, y});
        pixels.push_back({y, x});
        pixels.push_back({-x, y});
        pixels.push_back({-y, x});
        pixels.push_back({-x, -y});
        pixels.push_back({-y, -x});
        pixels.push_back({x, -y});
        pixels.push_back({y, -x});

        if (d < 0) {
            d += incE;
            incE += 2;
            incSE += 2;
        } else {
            d += incSE;
            incE += 2;
            incSE += 4;
            y--;
        }
        x++;
    }

    for (const auto& pixel : pixels) {
        drawPixel(cx + pixel.first, cy + pixel.second, preto);
    }
    
}

/*
Função para preencher um polígono
*/
void preencherPoligono(forma& f) {
    // Marca poligono como preenchido
    f.preenchido = true;
    
    // Cria Tabela de Arestas (TA), Tabela de Arestas Ativas (TAA) e Span Buffer
    struct Aresta {
        int yMax;
        float xMin;
        float inversoDeclive;
    };
    std::vector<std::vector<Aresta>> TA(height);
    std::vector<Aresta> TAA;

    // Preenche a Tabela de Arestas
    int ymin = height;
    for (auto verticeAtual = f.v.begin(); verticeAtual != f.v.end(); ++verticeAtual) {
        auto proximoVertice = std::next(verticeAtual);
        if (proximoVertice == f.v.end()) {
            proximoVertice = f.v.begin();
        }

        int y1 = verticeAtual->y, y2 = proximoVertice->y;
        int x1 = verticeAtual->x, x2 = proximoVertice->x;

        if (y1 == y2) continue; // Ignora arestas horizontais

        if (y1 > y2) {
            std::swap(y1, y2);
            std::swap(x1, x2);
        }

        Aresta aresta;
        aresta.yMax = y2;
        aresta.xMin = x1;
        aresta.inversoDeclive = static_cast<float>(x2 - x1) / (y2 - y1);

        if (y1 < height) {
            TA[y1].push_back(aresta);
        }
        ymin = std::min(ymin, y1);
    }

    // Inicializa y como o valor da menor ordenada de todos os vértices do polígono
    int y = ymin;

    // Repetir até que tanto a TA como a TAA fiquem vazias
    while (y < height && (!TA[y].empty() || !TAA.empty())) {
        // Mover da TA para a TAA as arestas para as quais y = ymin
        if (y < TA.size()) {
            for (const auto& aresta : TA[y]) {
                TAA.push_back(aresta);
            }
            TA[y].clear();
        }

        // Ordenar a TAA pelo x(ymin)
        std::sort(TAA.begin(), TAA.end(), [](const Aresta& a, const Aresta& b) {
            return a.xMin < b.xMin;
        });

        // Preencher os spans do scanline corrente
        for (size_t i = 0; i + 1 < TAA.size(); i += 2) {
            int xStart = static_cast<int>(TAA[i].xMin);
            int xEnd = static_cast<int>(TAA[i + 1].xMin);
            for (int x = xStart; x <= xEnd; ++x) {
                if (y < height) {
                    f.preenchidos.push_front({x, y});
                }
            }
        }

        // Remover da TAA todas as arestas para as quais y é igual a ymax - 1
        TAA.erase(std::remove_if(TAA.begin(), TAA.end(), [y](const Aresta& aresta) {
            return y == aresta.yMax - 1;
        }), TAA.end());

        // Incrementar o valor de y de uma unidade
        ++y;

        // Incrementar de 1/m o valor de x das arestas existentes na TAA
        for (auto& aresta : TAA) {
            aresta.xMin += aresta.inversoDeclive;
        }
    }
}

// Função para ler a cor de um pixel na janela GLUT
void lerCorPixel(int x, int y, unsigned char* cor) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        // Coordenadas fora dos limites da janela
        cor[0] = cor[1] = cor[2] = 0;
        return;
    }
    glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, cor);
}

// Função para comparar duas cores
bool coresIguais(unsigned char* cor1, unsigned char* cor2) {
    return cor1[0] == cor2[0] && cor1[1] == cor2[1] && cor1[2] == cor2[2];
}


void floodFill(int x, int y, unsigned char* novaCor, forma& f) {
    unsigned char corAlvo[3];
    lerCorPixel(x, y, corAlvo);

    if (coresIguais(corAlvo, novaCor)) return;

    // Marca os pixels visitados
    std::vector<std::vector<bool>> visitado(height, std::vector<bool>(width, false));
    
    // Fila para expansão
    std::queue<std::pair<int, int>> fila;
    fila.push({x, y});
    visitado[y][x] = true;

    while (!fila.empty()) {
        int cx = fila.front().first;
        int cy = fila.front().second;
        fila.pop();

        unsigned char corAtual[3];

        // Varredura horizontal para encontrar os limites
        int esquerda = cx;
        int direita = cx;

        // Procura para a esquerda
        while (esquerda > 0) {
            lerCorPixel(esquerda - 1, cy, corAtual);
            if (!coresIguais(corAtual, corAlvo)) break;
            esquerda--;
        }

        // Procura para a direita
        while (direita < width - 1) {
            lerCorPixel(direita + 1, cy, corAtual);
            if (!coresIguais(corAtual, corAlvo)) break;
            direita++;
        }

        // Desenha e processa os pixels entre `esquerda` e `direita`
        for (int nx = esquerda; nx <= direita; nx++) {
            int corInt[3] = {novaCor[0], novaCor[1], novaCor[2]};
            drawPixel(nx, cy, corInt);
            f.preenchidos.push_front({nx, cy, {novaCor[0], novaCor[1], novaCor[2]}});

            // Adiciona os vizinhos verticais à fila
            if (cy > 0 && !visitado[cy-1][nx]) {
                lerCorPixel(nx, cy-1, corAtual);
                if (coresIguais(corAtual, corAlvo)) {
                    fila.push({nx, cy-1});
                    visitado[cy-1][nx] = true;
                }
            }
            if (cy < height - 1 && !visitado[cy+1][nx]) {
                lerCorPixel(nx, cy+1, corAtual);
                if (coresIguais(corAtual, corAlvo)) {
                    fila.push({nx, cy+1});
                    visitado[cy+1][nx] = true;
                }
            }
        }
    }
}



// Função para preencher todas as formas geométricas rasterizadas
void preencherTodasFormas() {
    unsigned char corNova[3] = {0, 0, 255}; // Azul

    for (auto& f : formas) {
        if (f.preenchido || f.tipo == LIN) {
            continue;
        }
        else if (f.tipo == CIR) {
            // O centro do circulo é o segundo vértice
            int i = 0, x[2], y[2];
            for (auto& v : f.v) {
                x[i] = v.x;
                y[i] = v.y;
                ++i;
            }
            floodFill(x[1], y[1], corNova, f);
            f.preenchido = true; 
            f.cor[0] = corNova[0];
            f.cor[1] = corNova[1];
            f.cor[2] = corNova[2];
        } else {
            vertice centroide = calcularCentroide(f);
            floodFill(centroide.x, centroide.y, corNova, f);
            f.preenchido = true; // Marca a forma como preenchida
            f.cor[0] = corNova[0];
            f.cor[1] = corNova[1];
            f.cor[2] = corNova[2];
        }
    }
}
