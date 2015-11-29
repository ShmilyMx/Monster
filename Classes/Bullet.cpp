#include "Bullet.h"

bool Bullet::init() {
	do
	{
		CC_BREAK_IF(!Sprite::initWithFile("Projectile.png"));
		m_clockedEnemy = NULL;

		return true;
	} while (0);
	return false;
}