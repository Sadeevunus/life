#include <iostream>
#include <array>
#include <ctime>
#include <vector>
#include <algorithm>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string>
#include <cmath>
#include <SDL_image.h>


const int G_W = 480;
const int G_H = 270;
const int fps = 1000 / 24;

int mouse_x = 0; int mouse_y = 0;
int mouse_px; int mouse_py;
bool running = true; bool right = false; bool left = false; bool press = false; bool quit = false;
int stopped = -1;

SDL_Event e;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Window* window1;
SDL_Surface* surface1;

std::array<std::array<int, G_H>, G_W> display{};
std::array<std::array<int, G_H>, G_W> swap{};
std::vector<SDL_FPoint> points;
std::vector<SDL_FPoint> points1;
std::vector<SDL_Color> colors;


SDL_Window* gwindow;
SDL_Renderer* grenderer = NULL;
TTF_Font* gfont = NULL;
SDL_Event gEvent;
SDL_Texture* mTexture;



class Ttexture              //этот класс писал не я
{
public:
    Ttexture();
    ~Ttexture();
    
    bool loadFromRenderedText(std::string textureText, SDL_Color textColor);

    void free();
    void setColor(Uint8 red, Uint8 green, Uint8 blue);
    void setBlendMode(SDL_BlendMode blending);
    void setAlpha(Uint8 alpha);
    void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);
    int getWidth();
    int getHeight();

private:

    int mWidth;
    int mHeight;
};
Ttexture gTtexture;
Ttexture::Ttexture(){
    mTexture = NULL;
    mWidth = 0;
    mHeight = 0;
}
Ttexture::~Ttexture(){
    free();
}
bool Ttexture::loadFromRenderedText(std::string textureText, SDL_Color textColor){
    free();
    SDL_Surface* textSurface = TTF_RenderText_Solid(gfont, textureText.c_str(), textColor);
    if (textSurface == NULL){
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
    }
    else{
        mTexture = SDL_CreateTextureFromSurface(grenderer, textSurface);
        if (mTexture == NULL){
            printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
        }
        else{
            mWidth = textSurface->w;
            mHeight = textSurface->h;
        }

        SDL_FreeSurface(textSurface);
    }

    return mTexture != NULL;
}
void Ttexture::setColor(Uint8 red, Uint8 green, Uint8 blue){
    SDL_SetTextureColorMod(mTexture, red, green, blue);
}
void Ttexture::setBlendMode(SDL_BlendMode blending){
    SDL_SetTextureBlendMode(mTexture, blending);
}
void Ttexture::setAlpha(Uint8 alpha){
    SDL_SetTextureAlphaMod(mTexture, alpha);
}
void Ttexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip){
    SDL_Rect renderQuad = { x, y, mWidth, mHeight };

    if (clip != NULL){
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }
    SDL_RenderCopyEx(grenderer, mTexture, clip, &renderQuad, angle, center, flip);
}
int Ttexture::getWidth(){
    return mWidth;
}
int Ttexture::getHeight(){
    return mHeight;
}
bool init(){
    bool success = true;

    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        success = false;
    }
    else{
        if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")){
            printf("Warning: Linear texture filtering not enabled!");
        }

        gwindow = SDL_CreateWindow("Info", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, SDL_WINDOW_SHOWN);
        if (gwindow == NULL){
            printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
            success = false;
        }
        else{
            grenderer = SDL_CreateRenderer(gwindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (grenderer == NULL){
                printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
                success = false;
            }
            else{
                SDL_SetRenderDrawColor(grenderer, 0, 0, 0, 255);
                int imgFlags = IMG_INIT_PNG;
                if (!(IMG_Init(imgFlags) & imgFlags)){
                    printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
                    success = false;
                }

                if (TTF_Init() == -1){
                    printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
                    success = false;
                }
            }
        }
    }return success;
}
bool loadMedia(std::string text, int r,int g,int b) {
    bool success = true;

    gfont = TTF_OpenFont("Amiga4ever.ttf", 28);
    if (gfont == NULL){
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        success = false;
    }
    else{
        SDL_Color textColor = {r, g, b };
        if (!gTtexture.loadFromRenderedText(text, textColor)){
            printf("Failed to render text texture!\n");
            success = false;
        }
    }return success;
}
void Ttexture::free(){
    if (mTexture != NULL){
        SDL_DestroyTexture(mTexture);
        mTexture = NULL;
        mWidth = 0;
        mHeight = 0;
    }
}
void close(){
    gTtexture.free();

    TTF_CloseFont(gfont);
    gfont = NULL;

    SDL_DestroyRenderer(grenderer);
    SDL_DestroyWindow(gwindow);
    gwindow = NULL;
    grenderer = NULL;

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}


class Button {
public:
    SDL_Rect srect, drect;
    bool hover = false;
    SDL_Texture* tex;
    //SDL_Rect Mouse{ mouse_x,mouse_y, 1,1 };
    Button(std::string text, int r, int g, int b);
    ~Button();
    void update(SDL_Rect srect, int mouse_x, int mouse_y);
    void draw();
    bool hovered();
};

Button::Button(std::string text, int r, int g, int b) {
    static SDL_Texture* t = mTexture;
    tex = t;
    loadMedia(text, r, g, b);                       // Error :(
    //srect.h = 100; srect.w = 180;
    drect.h = 150; drect.w = 270;
    
}
Button::~Button() {

}
bool Button::hovered() {
    return hover;
}
void Button::update(SDL_Rect drect, int mouse_x, int mouse_y) {
    SDL_Rect mouse{mouse_x,mouse_y,1,1};
    if (SDL_HasIntersection(&drect , &mouse)) {
        hover = true;
        std::cout << "hovered \n";
        //srect.x=960;
    }
    else {
        hover = false;
        //srect.x=960;
    }
}
void Button::draw() {
    SDL_RenderCopy(grenderer, tex, NULL, &drect);             //
}
Button Play("Play",255,255,255);


void a(const char *A) {
    //std::cout << A << " is alive" << '\n'; 
}
void drawpixel(float x, float y, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255) {
    points.emplace_back(x, y);
    colors.emplace_back(r, g, b, a);
}
bool isAlive(std::array<std::array<int, G_H>, G_W> &life , int x, int y) {
    int alive = 0;
    

    if (x > 0 and y<G_H) { if ((life[x - 1][y]) == 1) { alive += 1; a("left"); } }
    if (x < G_W-1) { if ((life[x + 1][y]) == 1) { alive += 1; a("right"); } }
    if (y > 0 and x<G_W) {if ((life[x][y - 1]) == 1) { alive += 1;  a("top"); } }
    if (x<G_W and y < G_H-1) { if ((life[x][y + 1]) == 1) { alive += 1; a("bottom"); } }

    if (y > 0 and x > 0) { if (life[x - 1][y - 1] == 1) { alive += 1; a("left top"); } }
    if (y > 0 and x < G_W-1) { if (life[x + 1][y - 1] == 1) { alive += 1; a("right top"); } }
    if (y < G_H -1 and x > 0) { if (life[x - 1][y + 1] == 1) { alive += 1; a("left bottom"); } }
    if (y < G_H -1 and x < G_W-1 and (life[x + 1][y + 1] == 1)) { alive += 1; a("right bottom"); }


    if (alive == 2 and life[x][y] == 1) { return true; }
    if (alive == 3) { return true; }

    return false;
}
void mousedraw(int& x, int& y) {
    int rad = 12;
    for (int xx = x - rad; x > rad and x < G_W-1 - rad and xx < x + rad; xx+=3) {
        for (int yy = y - rad; y > rad and y < G_H-1 - rad and yy < y + rad; yy+=1) {
            drawpixel(xx, yy);
            display[xx][yy] = 1;
        }
    }
}

void mouseerase(int& x, int& y) {
    std::vector<SDL_FPoint> points1;
    int rad = 12;
    for (int xx = x - rad; x > rad and x < G_W-1 - rad and xx < x + rad; xx++) {
        for (int yy = y - rad; y > rad and y < G_H-1 - rad and yy < y + rad; yy++) {
            points1.emplace_back(xx, yy);
            display[xx][yy] = 0;
        }
    }
}
void clearpixels() {
    points.clear();
}
void clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}
void update() {

    mouse_px = mouse_x;
    mouse_py = mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    mouse_x /= 4;
    mouse_y /= 4;
    clear();

    if (press and left) {
        mousedraw(mouse_x, mouse_y);
    }
    else if (press and right) {
        mouseerase(mouse_x, mouse_y);
    }


    for (int i = 0; i < points.size(); i++) {
        SDL_SetRenderDrawColor(renderer, colors[i].r, colors[i].g, colors[i].b, colors[i].a);
        SDL_RenderDrawPointF(renderer, points[i].x, points[i].y);
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (int i = 0; i < points1.size(); i++) {
        SDL_RenderDrawPointF(renderer, points1[i].x, points1[i].y);
    }

    SDL_RenderPresent(renderer);
}
void restart() {
    for (auto& row : display) {
        std::generate(row.begin(), row.end(), []() {return 0; });
    }
}
void randfill() {
    for (auto& row : display) {
        std::generate(row.begin(), row.end(), []() {return ((rand() % 10 == 0) ? 1 : 0); });
    }
}
void quit1() {
    running = false;
    SDL_Delay(125);
}

void input() {
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = false;
            SDL_Delay(175);
            SDL_Quit();
            exit(0);
        }
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (SDL_PRESSED == e.button.state) { press = true; }
            switch (e.button.button) {
            case SDL_BUTTON_LEFT:
                left = true;
                right = false;
                break;
            case SDL_BUTTON_RIGHT:
                right = true;
                left = false;
                break;
            }
        }
        if(e.type == SDL_MOUSEBUTTONUP){ press = false;}
        if (e.type == SDL_KEYUP) {
            if (e.key.keysym.sym == SDLK_s) { stopped = -stopped; break; }
            if (e.key.keysym.sym == SDLK_d) { restart(); break; }
            if (e.key.keysym.sym == SDLK_r) { randfill(); break; }
            if (e.key.keysym.sym == SDLK_q) { quit1(); break; }
        }
    }
}

void menu() {
    Play.update(Play.drect,mouse_x,mouse_y);
    if (SDL_PollEvent(&gEvent)) {
        if (gEvent.type == SDL_QUIT) {
            quit = true;
            //SDL_RenderPresent(renderer);
        }
        if (e.button.button == SDL_BUTTON_LEFT) { if (Play.hovered()) { std::cout << "omg"; } }
    }
    SDL_SetRenderDrawColor(grenderer, 250, 215, 90, 255);
    SDL_RenderClear(grenderer);
    //SDL_UpdateWindowSurface(gwindow);
    SDL_SetRenderDrawColor(grenderer, 255, 255, 255, 255);
    std::cout << Play.drect.x <<" " << Play.drect.y<<'\n';
    SDL_RenderDrawLine(grenderer, 0, 0, Play.drect.x, Play.drect.y);
    //gTtexture.render(Play.drect.x, Play.drect.y);
    Play.draw();

    SDL_RenderPresent(grenderer);
}


int main(int argc, char* argv[]) {
    srand(time(0));
    Ttexture gTtexture;

    init();

    std::string info_text = "IF SOMETHING ISN\'T WORKING - ALT+TAB \n S - stop/continue the game  \n D - delete the game \n R - new random game \n Q - quit the game \n LMB - draw  \n RMB - erase \n";

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(480 * 4, 270 * 4, SDL_WINDOW_MAXIMIZED, &window, &renderer);
    SDL_RenderSetScale(renderer, 4, 4);
    //SDL_CreateWindow("update soon", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 270, 0);
    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "info", info_text, NULL);
    //SDL_CreateWindowAndRenderer(480 * 4, 270 * 4, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALWAYS_ON_TOP, &gwindow, &grenderer);
    Play.drect.x = 845;
    Play.drect.y = 445;      //

    bool quit = false;

    for (auto& row : display) {
        std::generate(row.begin(), row.end(), []() {return ((rand() % 10 == 0) ? 1 : 0); });
    }
    
    std::cout << ((display[479][269])? "you are that 1 dientist that hates all 9 your colleagues :)":"you are one of the (9 out of 10 dientists)") << '\n';
    
    gTtexture.setAlpha(255);
    gTtexture.setBlendMode(SDL_BLENDMODE_BLEND);
    gTtexture.setColor(255,255,255);

    if (!quit) menu();
    while (running) {
        if(stopped>0){
            for (int x = 0; x < G_W; x++) {
                for (int y = 0; y < G_H; y++) {
                    swap[x][y] = isAlive(display, x, y)?  1 : 0 ;
                }
            }
        }
        else if (stopped<0){
            for (int x = 0; x < G_W; x++) {
                for (int y = 0; y < G_H; y++) {
                    swap[x][y] = display[x][y];
                }
            }
        }
        std::copy(swap.begin(), swap.end(), display.begin());
        input();
        
        for (int x = 0; x < G_W; x++) {
            for (int y = 0; y < G_H; y++) {
                if (swap[x][y]==1) {
                    drawpixel(x, y);
                }
            }
        }
        update();
        SDL_Delay(fps);
        clearpixels();
        
    }
    return 0;
}
