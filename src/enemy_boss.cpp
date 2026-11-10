#include "enemy_boss.h"
#include "globals.h"
#include <cmath>
#include <cstdlib>

Enemy_boss::Enemy_boss(){
	loadSprite(asset("graphics/enemy_boss.png"));
	characterSpeed = .5;

	source = sf::Vector2i(0, Down);
}
void Enemy_boss::movement(Character* player, sf::RenderWindow* window){
	net_x = player->characterImage.getPosition().x - characterImage.getPosition().x;
	net_y = player->characterImage.getPosition().y - characterImage.getPosition().y;
	
	if (std::abs(net_x) < std::abs(net_y)){
		if (net_x > 0){
			source.y = Right;
		}
		else if (net_x < 0){
			source.y = Left;
		}
		else{
			if (net_y > 0){
				source.y = Down;
			}
			else{
				source.y = Up;
			}
		}
		if (!col.isCollisionMap(characterImage.getPosition(), source.y)|| colSpecialTile()){
			if (net_x > 0){
				characterImage.move({ (float)(characterSpeed), (float)(0) });
			}
			else if (net_x < 0){
				characterImage.move({ (float)(-characterSpeed), (float)(0) });
			}
			else{
				if (net_y > 0){
					characterImage.move({ (float)(0), (float)(characterSpeed) });
				}
				else{
					characterImage.move({ (float)(0), (float)(-characterSpeed) });
				}
			}
		}
		else
		{
			if (source.y == Right || source.y == Left){
				if (net_y > 0){
					source.y = Down;
				}
				else {
					source.y = Up;
				}
			}
			if (!col.isCollisionMap(characterImage.getPosition(), source.y) || colSpecialTile()){
				if (net_y > 0){
					characterImage.move({ (float)(0), (float)(characterSpeed) });
				}
				else {
					characterImage.move({ (float)(0), (float)(-characterSpeed) });
				}
			}
		}
	}
	else
	{
		if (net_y > 0){
			source.y = Down;
		}
		else if (net_y < 0){
			source.y = Up;
		}
		else{
			if (net_x > 0){
				source.y = Right;
			}
			else{
				source.y = Left;
			}
		}
		if (!col.isCollisionMap(characterImage.getPosition(), source.y) || colSpecialTile()){
			if (net_y > 0){
				characterImage.move({ (float)(0), (float)(characterSpeed) });
			}
			else if (net_y < 0){
				characterImage.move({ (float)(0), (float)(-characterSpeed) });
			}
			else{
				if (net_x > 0){
					characterImage.move({ (float)(characterSpeed), (float)(0) });
				}
				else {
					characterImage.move({ (float)(-characterSpeed), (float)(0) });
				}
			}
		}
		else
		{
			if (source.y == Down || source.y == Up){
				if (net_x > 0){
					source.y = Right;
				}
				else {
					source.y = Left;
				}
			}
			if (!col.isCollisionMap(characterImage.getPosition(), source.y) || colSpecialTile()){
				if (net_x > 0){
					characterImage.move({ (float)(characterSpeed), (float)(0) });
				}
				else {
					characterImage.move({ (float)(-characterSpeed), (float)(0) });
				}
			}
		}
	}
		
		frameCounter += frameSpeed * clock.restart().asSeconds();
		if (frameCounter > switchFrame){
			frameCounter = 0;
			source.x++;
			if (source.x * 24 >= (int)cTexture.getSize().x)
				source.x = 0;
		}
		setFrame();
		window->draw(characterImage);
		if (game_state == 1){
			if (std::abs(net_x) < 150 && std::abs(net_y) < 150){
				if (player->beep_sound.getStatus() != sf::SoundSource::Status::Playing){
					//player->beep_sound.setPlayingOffset(sf::seconds(3.0f));
					player->beep_sound.play();
				}
			}
			else
				player->beep_sound.stop();
		}
		else
			player->beep_sound.stop();

	}
	int Enemy_boss::colSpecialTile(){
		sf::Vector2i a = current_map->at(col.next_tile1.x, col.next_tile1.y);
		sf::Vector2i b = current_map->at(col.next_tile2.x, col.next_tile2.y);
		if ((a.y > 0 && a.x != 13) || (b.y > 0 && b.x != 13))
			return 0;//cannot pass	
		return 1; //can pass
	}