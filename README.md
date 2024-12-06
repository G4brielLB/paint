# Paint - Computação Gráfica

Este projeto é uma aplicação de pintura gráfica utilizando OpenGL. Ele permite desenhar várias formas geométricas, como linhas, triângulos, quadriláteros, polígonos e círculos, além de aplicar transformações geométricas e preenchimento de formas.

## Funcionalidades

### Desenho de formas geométricas:
- Linha
- Triângulo
- Quadrilátero
- Polígono com 4 ou mais vértices
- Círculo

### Transformações geométricas:
- Translação
- Escala
- Rotação
- Cisalhamento
- Reflexão

### Preenchimento de formas:
- Preenchimento de polígonos
- Flood fill para todas as formas

## Dependências
- OpenGL
- GLUT

## Compilação e Execução

### Compilação no macOS
1. Instale as dependências necessárias:
2. Compile o código:
3. Execute o programa:

### Compilação no Linux
1. Instale as dependências necessárias:
2. Compile o código:
3. Execute o programa:

## Uso

### Controles do Mouse
- **Botão Esquerdo**: Usado para desenhar formas geométricas.
    - Clique para definir os pontos das formas.
    - Para polígonos, clique para adicionar vértices.

### Controles do Teclado
- **ESC**: Sair do programa.
- **ENTER**: Confirmar a criação de um polígono com 4 ou mais vértices.
- **p**: Preencher o último polígono desenhado.
- **f**: Preencher todas as formas desenhadas.

### Transformações Geométricas
- **w**: Transladar para cima.
- **s**: Transladar para baixo.
- **a**: Transladar para a esquerda.
- **d**: Transladar para a direita.
- **e**: Aumentar a escala.
- **E**: Diminuir a escala.
- **r**: Rotacionar no sentido horário.
- **R**: Rotacionar no sentido anti-horário.
- **c**: Cisalhar na direção x.
- **C**: Cisalhar na direção -x.
- **y**: Cisalhar na direção y.
- **Y**: Cisalhar na direção -y.
- **v**: Refletir verticalmente.
- **h**: Refletir horizontalmente.

### Menu Pop-up
Clique com o botão direito do mouse para abrir o menu pop-up e selecionar o tipo de forma a ser desenhada:
- Linha
- Quadrilátero
- Triângulo
- Polígono
- Círculo
- Sair

## Estrutura do Código
- **main.cpp**: Contém a implementação principal do programa, incluindo a lógica de desenho, transformações geométricas e preenchimento de formas.
- **glut_text.h**: Biblioteca auxiliar para desenhar texto na janela GLUT/OpenGL.

## Autor
Gabriel Lopes Bastos

## Licença
Este projeto está licenciado sob a licença MIT. Consulte o arquivo LICENSE para obter mais informações.
