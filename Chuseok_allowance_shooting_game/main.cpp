/**
*  TODO List
* 1) 아이템 : 속도증가, 데미지 증가
* 2) 총알 시스템 개편
* 3) sound effect 개선
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>//SoundBuffer 사용
#include <SFML/Audio/SoundSource.hpp>
#include <Windows.h>
//#include <Windows.h>// sleep() 사용


using namespace sf;

struct Player {
	RectangleShape sprite;// sprite : 그림이 되는 부분
	int speed;// 이속
	int speed_max;// 이속 최대치
	int score;
	int life;
	float x, y;// player 좌표
};


//총알
struct Bullet {
	RectangleShape sprite;
	int is_fired;// 발사 여부
};

struct Enemy {
	RectangleShape sprite;
	int speed;
	int life;
};

// 열거형
enum item_type {
	SPEED,// 0
	DELAY// 1
};

struct Item {
	RectangleShape sprite;
	int delay;// 쿨타임
	int is_presented;//아이템이 나타났는가
	long presented_time;//나타난 시간
	enum item_type type;// 아이템 타입
};

struct Textures {
	Texture bg; // 배경 이미지
	Texture enemy;// 적 이미지
	Texture gameover;// 게임 오버 이미지
	Texture item_delay;// 공속 아이템 이미지
	Texture item_speed;// 이속 아이템 이미지
	Texture player;// 플레이어 이미지
	Texture bullet;// 총알 이미지
};

struct SBuffers {
	SoundBuffer BGM;
	SoundBuffer rumble;
	SoundBuffer item;
	SoundBuffer gameover;
};

struct Gameover {
	Sprite sprite;
};

//obj1과 obj2의 충돌 여부 충돌하면 1로 반환 아니면 0으로 반환
int is_collide(RectangleShape obj1, RectangleShape obj2) {
	return obj1.getGlobalBounds().intersects(obj2.getGlobalBounds());
}

// 움직이는 함수
void moving(struct Player player)
{
	if (Keyboard::isKeyPressed(Keyboard::Left))
	{
		player.sprite.move(-1 * player.speed, 0);//왼쪽 이동
	}
	if (Keyboard::isKeyPressed(Keyboard::Up))
	{
		player.sprite.move(0, -1 * player.speed);//위쪽 이동
	}
	if (Keyboard::isKeyPressed(Keyboard::Down))
	{
		player.sprite.move(0, player.speed);//아래쪽 이동
	}
	if (Keyboard::isKeyPressed(Keyboard::Right))
	{
		player.sprite.move(player.speed, 0);//오른쪽 이동
	}
}

// 전역변수 - const로 처리하여 중간에 값을 바꿀 수 없는 것만 전역변수로 세팅
const int ENEMY_NUM = 10;// 적의 최대 갯수
const int BULLET_NUM = 50;// 총알 최대 갯수
const int ITEM_NUM = 2;// 아이템의 최대 종류
const int W_WIDTH = 1280, W_HEIGHT = 575;// 창의 크기
const int GO_WIDTH = 880, GO_HEIGHT = 468;// 게임오버 그림의 크기

int main(void) {

	// console 제거
	/*
	#ifdef WIN32
		HWND hwnd = GetConsoleWindow();
		ShowWindow(hwnd, SW_HIDE);
	#endif
	*/

	//이미지
	struct Textures t;
	t.bg.loadFromFile("./resources/image/background.jpg");
	t.enemy.loadFromFile("./resources/image/enemy.png");
	t.gameover.loadFromFile("./resources/image/gameover.png");
	t.player.loadFromFile("./resources/image/player.png");
	t.item_delay.loadFromFile("./resources/image/item_damage.png");
	t.item_speed.loadFromFile("./resources/image/item_speed.png");
	t.bullet.loadFromFile("./resources/image/bullet.png");

	// BGM
	struct SBuffers sb;
	sb.BGM.loadFromFile("./resources/sound/bgm.ogg");
	sb.rumble.loadFromFile("./resources/sound/rumble.flac");// 적 죽을 때 효과음
	sb.item.loadFromFile("./resources/sound/item.flac");// item얻을 때 효과음
	// TODO : 게임종료됐을 때 브금 틀기
	sb.gameover.loadFromFile("./resources/sound/game_over.wav");// 게임이 종료되었을 떄의 효과음

	srand(time(0));//랜덤 함수 사용

	//640 x 480 윈도우 화면 나옴
	//잠깐 떴다가 사라지는 건 return 0때문에 프로그램이 종료된 것
	RenderWindow window(VideoMode(W_WIDTH, W_HEIGHT), "AfterSchool");
	window.setFramerateLimit(60);//1초에 60장 보여준다. 플레이어가 빨리 가지 않도록 하기

	long start_time = clock();// 게임 시작 시간
	long spent_time;// 게임 진행 시간
	long fired_time = 0;// 최근 총알이 발사된 시간
	int is_gameover = 0;

	// BGM
	Sound BGM_sound;
	BGM_sound.setBuffer(sb.BGM);
	BGM_sound.setLoop(1);// BGM 무한 반복
	BGM_sound.play();

	// text 폰트
	Font font;
	font.loadFromFile("C:\\Windows\\Fonts\\Candara.ttf");//C드라이브에 있는 폰트 가져오기

	Text text;
	char info[40];
	text.setFont(font);//폰트 세팅
	text.setCharacterSize(24);//폰트 크기
	text.setFillColor(Color(255, 255, 255));//RGB로 흰색 표현
	text.setPosition(0, 0);//텍스트 위치 0,0

	//배경
	Sprite bg_sprite;
	bg_sprite.setTexture(t.bg);
	bg_sprite.setPosition(0, 0);

	//gameover
	struct Gameover gameover;
	Sound gameover_sound;
	gameover.sprite.setTexture(t.gameover);
	gameover.sprite.setPosition(0, 0);//game over 그림 가운데 나타내기
	//gameover_sound.setBuffer(sb.gameover);

	// 플레이어
	struct Player player;
	player.sprite.setTexture(&t.player);// 포인터를 넘겨주기 때문에 주소값 넘겨주기
	player.sprite.setSize(Vector2f(150, 150));//플레이어 사이즈
	player.sprite.setPosition(100, 100);//플레이어 시작 위치
	player.x = player.sprite.getPosition().x;// x좌표
	player.y = player.sprite.getPosition().y;// y좌표
	player.speed = 7;//플레이어 속도
	player.speed_max = 10;// 이속 최대값
	player.score = 0;//플레이어 초기 점수
	player.life = 1;// 플레이어 생명

	// 총알
	int bullet_speed = 20;// 총알 속도
	int bullet_idx = 0;// 발사될 때마다 인덱스 증가시킬 것
	int bullet_delay = 500;	// 총알의 delay는 모두의 속성이므로 struct에 넣지 않음. 0.5초마다 나감
	int bullet_delay_max = 100;// 총알의 delay 최대값
	Sound bullet_sound;
	bullet_sound.setBuffer(sb.rumble);

	struct Bullet bullet[BULLET_NUM];
	for (int i = 0; i < BULLET_NUM; i++)
	{
		bullet[i].sprite.setTexture(&t.bullet);// 포인터를 넘겨주기 때문에 주소값 넘겨주기
		bullet[i].sprite.setSize(Vector2f(40, 40));// 총알 크기
		bullet[i].sprite.setPosition(player.x + 110, player.y + 20);// 총알 초기 위치
		bullet[i].is_fired = 0;// 총알 발사 여부 (0:false, 1:true)
	}

	// enemy
	struct Enemy enemy[ENEMY_NUM];
	Sound enemy_explosion_sound;
	enemy_explosion_sound.setBuffer(sb.rumble);
	int enemy_score = 100;
	int enemy_respawn_time = 8;

	/* enemy update */
	for (int i = 0; i < ENEMY_NUM; i++)
	{
		enemy[i].sprite.setTexture(&t.enemy);// 포인터를 넘겨주기 때문에 주소값 넘겨주기
		enemy[i].sprite.setSize(Vector2f(70, 70));// 적 크기
		enemy[i].sprite.setPosition(rand() % 300 + W_WIDTH * 0.9, rand() % 380);// 가로 화면의 90%부터 적들이 나옴
		enemy[i].life = 1;// 적의 생명
		enemy[i].speed = -(rand() % 5 + 1);// 랜덤으로 주어지는 적의 속도
	}

	// item의 고유 특성
	struct Item item[ITEM_NUM];
	Sound item_sound;
	item_sound.setBuffer(sb.item);
	item[0].sprite.setTexture(&t.item_speed);// 이속 이미지 주소 설정
	item[0].delay = 25000;// 25초마다 이속 아이템 나옴
	item[0].type = SPEED;
	item[1].sprite.setTexture(&t.item_delay);// 공속 이미지 주소 설정
	item[1].delay = 20000;// 20초마다 공속 아이템 나옴
	item[1].type = DELAY;

	for (int i = 0; i < ITEM_NUM; i++)
	{
		// item의 공통 특성
		item[i].sprite.setSize(Vector2f(70, 70));
		item[i].is_presented = 0;
		item[i].presented_time = 0;// 아이템이 뜨지 않은 상태
	}

	//유지 시키는 방법은? -> 무한 반복
	while (window.isOpen()) //윈도우창이 열려있는 동안 계속 반복
	{
		spent_time = clock() - start_time;// 시간이 지남에 따라 증가

		//총알이 플레이어 따라다닐 수 있도록 
		player.x = player.sprite.getPosition().x;	//플레이어 x좌표
		player.y = player.sprite.getPosition().y;	//플레이어 y좌표

		Event event;//이벤트 생성
		while (window.pollEvent(event)) //이벤트가 발생. 이벤트가 발생해야 event 초기화가 됨
		{
			switch (event.type)
			{
				//종료(x)버튼을 누르면 Event::Closed(0) 
			case Event::Closed://정수임
				window.close();//윈도우창이 닫힘
				break;
			}
		}


		/* game 상태 update */
		if (player.life <= 0)
		{
			is_gameover = 1;// 1 == true
		}

		/* player update */
		//방향키
		if (player.life > 0)
		{
			if (Keyboard::isKeyPressed(Keyboard::Left))
			{
				player.sprite.move(-1 * player.speed, 0);//왼쪽 이동
			}
			if (Keyboard::isKeyPressed(Keyboard::Up))
			{
				player.sprite.move(0, -1 * player.speed);//위쪽 이동
			}
			if (Keyboard::isKeyPressed(Keyboard::Down))
			{
				player.sprite.move(0, player.speed);//아래쪽 이동
			}
			if (Keyboard::isKeyPressed(Keyboard::Right))
			{
				player.sprite.move(player.speed, 0);//오른쪽 이동
			}
		}

		//player 이동 범위 제한
		if (player.sprite.getPosition().x < 0)
		{
			player.sprite.setPosition(0, player.sprite.getPosition().y);
		}
		else if (player.sprite.getPosition().x > W_WIDTH - 150)
		{
			player.sprite.setPosition(W_WIDTH - 150, player.sprite.getPosition().y);
		}
		if (player.sprite.getPosition().y < 0)
		{
			player.sprite.setPosition(player.sprite.getPosition().x, 0);
		}
		else if (player.sprite.getPosition().y > W_HEIGHT - 150)
		{
			player.sprite.setPosition(player.sprite.getPosition().x, W_HEIGHT - 150);
		}

		/* bullet update */
		// 총알 발사
		if (Keyboard::isKeyPressed(Keyboard::Space))
		{
			// delay가 클 때 작동 ( 현재 시간과 발사 시간이 0.5초만큼의 차이가 나면 총알 발사)
			if (spent_time - fired_time > bullet_delay)
			{
				//총알이 발사 되어있지 않다면 총알 발사
				if (!bullet[bullet_idx].is_fired && player.life > 0)
				{
					bullet[bullet_idx].sprite.setPosition(player.x + 110, player.y + 20);// 총알 초기 위치 (임시 테스트)
					bullet[bullet_idx].is_fired = 1;
					bullet_idx++;// 다음 총알 발사 가능하도록
					bullet_idx = bullet_idx % BULLET_NUM;// 50번 대신 idx가 0으로 바뀜
					bullet_sound.play();// 총알 발사 소리
					fired_time = spent_time;// 총알 장전 (총을 쏜 뒤에 총을 쏜 시점과 현재의 시점을 동일시할 것)
				}
			}
		}

		for (int i = 0; i < BULLET_NUM; i++)
		{
			if (bullet[i].is_fired) {
				bullet[i].sprite.move(bullet_speed, 0);
				if (bullet[i].sprite.getPosition().x > W_WIDTH)
					bullet[i].is_fired = 0;
			}
		}

		//enemy와의 충돌
		//intersects : 플레이어와 적 사이에서 교집합이 있는가
		for (int i = 0; i < ENEMY_NUM; i++)
		{
			// 10초 마다 enemy가 젠
			if (spent_time % (1000 * enemy_respawn_time) < 1000 / 60 + 1) // 1초동안 60프레임이 반복되기 때문에
			{
				// 게임이 진행중일때만 적이 나옴
				if (!is_gameover)
				{
					enemy[i].sprite.setSize(Vector2f(70, 70));
					enemy[i].sprite.setPosition(rand() % 300 + W_WIDTH * 0.9, rand() % 505);// 90%부터 적들이 나옴
					enemy[i].life = 1;
					// 10초마다 enemy 속도 +1
					enemy[i].speed = -(rand() % 3 + 1 + (spent_time / 1000 / enemy_respawn_time));
				}
			}

			if (enemy[i].life > 0)
			{
				// enemy와의 충돌
				if (is_collide(player.sprite, enemy[i].sprite) || is_collide(bullet[i].sprite, enemy[i].sprite))
				{
					enemy[i].life -= 1;//적의 생명 줄이기
					player.score += enemy_score;

					if (enemy[i].life == 0)
					{
						enemy_explosion_sound.play();
					}
				}

				// enemy의 x좌표가 0이 되면
				else if (enemy[i].sprite.getPosition().x < 0)
				{
					player.life -= 1;// player 목숨 1 차감
					enemy[i].life = 0;// 적 없애기
				}

				// 총알과 enemy의 충돌
				for (int j = 0; j < BULLET_NUM; j++)
				{
					if (is_collide(bullet[j].sprite, enemy[i].sprite))
					{
						if (bullet[j].is_fired)
						{
							enemy[i].life -= 1;
							player.score += enemy_score;

							if (enemy[i].life == 0)
							{
								enemy_explosion_sound.play();
							}
							bullet[j].is_fired = 0;
						}
					}
				}
				enemy[i].sprite.move(enemy[i].speed, 0);
			}
		}

		/* item update */
		for (int i = 0; i < ITEM_NUM; i++)
		{
			if (!item[i].is_presented && player.life > 0)// 아이템이 나타나지 않았고 플레이어의 체력이 있다면
			{
				if (spent_time - item[i].presented_time > item[i].delay)// 각각의 delay초보다 더 많은 시간이 지나면
				{
					item[i].sprite.setPosition(rand() % (W_WIDTH) * 0.8, rand() % W_HEIGHT * 0.8);
					item[i].is_presented = 1;// 아이템이 뜸 (true)
				}
			}
			if (item[i].is_presented)
			{
				// 아이템 획득 시 효과를 얻고 사라진다
				if (is_collide(player.sprite, item[i].sprite))
				{
					switch (item[i].type)
					{
					case SPEED: // 이속 아이템
						player.speed += 2;// player 속도 증가
						if (player.speed > player.speed_max)
						{
							player.speed = player.speed_max;
						}
						break;
					case DELAY: // 공속 아이템
						bullet_delay -= 100;// 총알 딜레이 줄이기
						if (bullet_delay < bullet_delay_max)
						{
							bullet_delay = bullet_delay_max;
						}
						break;
					}
					item_sound.play();
					// 사라지는 코드
					item[i].is_presented = 0;
					item[i].presented_time = spent_time;// 아이템을 먹는 순간 다시 쿨타임 시간 적용
				}
			}
		}

		// 시작 시간은 변하지 않음
		sprintf(info, "life: %d | score: %d | time: %d\n", player.life, player.score, spent_time / 1000);

		text.setString(info);

		//window.clear(Color::Black);//플레이어 자체 제거 (배경 지우기)
		window.draw(bg_sprite);

		for (int i = 0; i < ENEMY_NUM; i++)
		{
			if (enemy[i].life > 0)  window.draw(enemy[i].sprite);//적 보여주기
		}

		// 아이템 그려주기
		for (int i = 0; i < ITEM_NUM; i++)
		{
			if (item[i].is_presented)
				window.draw(item[i].sprite);
		}
		window.draw(player.sprite);//플레이어 보여주기(그려주기)
		window.draw(text);

		// 발사된 총알만 그리기
		for (int i = 0; i < BULLET_NUM; i++)
		{
			if (bullet[i].is_fired)
				window.draw(bullet[i].sprite);
		}
		int go = 0;// 별 의미 없는 변수. 0이면 실행 0초과이면 실행하지 않음
		if (is_gameover)
		{
			window.draw(gameover.sprite);
			// TODO : 게임이 멈추는 것을 구현할 것
			BGM_sound.stop();
			go++;
			//gameover_sound.setLoop(1);
			//if(gameover_sound.getStatus() == sf::SoundSource::Status::Stopped)
			//if ( go <= 1 )
				//gameover_sound.play();// gameover 효과음
		}

		window.display();
	}

	return 0;
}