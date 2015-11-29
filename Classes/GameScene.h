#pragma once
#include "cocos2d.h"
#include "Bullet.h"

USING_NS_CC;

class GameScene : public LayerColor {
public:

	static Scene *createScene();
	CREATE_FUNC(GameScene);
	virtual bool init() override;

	virtual void update(float) override;
private:
	Vector<Sprite *> m_enemies;
	Vector<Bullet *> m_bullets;

	void createEnemy(float);
	bool createBullet(Touch *, Event *);
	bool createFollowBullet(Touch *, Event *);
	void FollowEnemy(float);
};