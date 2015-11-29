#pragma once
#include "cocos2d.h"

USING_NS_CC;

class Bullet : public Sprite{
public:
	Bullet() {
		m_clockedEnemy = nullptr;
	}

	CREATE_FUNC(Bullet);
	virtual bool init() override;

	//被锁定的敌人
	CC_SYNTHESIZE(Sprite *, m_clockedEnemy, ClockedEnemy);
}; 