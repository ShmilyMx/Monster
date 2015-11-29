#include "GameScene.h"

#define origin Director::getInstance()->getVisibleOrigin()
#define visibleSize Director::getInstance()->getVisibleSize()

Scene *GameScene::createScene() {
	auto layer = GameScene::create();
	auto scene = Scene::create();
	scene->addChild(layer);
	return scene;
}

bool GameScene::init() {
	do
	{
		CC_BREAK_IF(!LayerColor::initWithColor(Color4B::WHITE));

		auto player = Sprite::create("Player.png");
		player->setPosition(origin + Vec2(player->getContentSize().width, visibleSize.height/2));
		this->addChild(player, 2, 1);

		auto listener = EventListenerTouchOneByOne::create();
		listener->onTouchBegan = CC_CALLBACK_2(GameScene::createFollowBullet, this);
		_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

		schedule(schedule_selector(GameScene::createEnemy), 1);
		scheduleUpdate();

		return true;
	} while (0);

	return false;
}

void GameScene::createEnemy(float) {
	auto enemy = Sprite::create("Target.png");
	//随机Y从屏幕右侧外出现敌人
	auto minY = enemy->getContentSize().height / 2;
	auto maxY = visibleSize.height - minY;
	auto x = visibleSize.width + enemy->getContentSize().width / 2;
	auto y = rand() % int((maxY - minY)) + minY;
	enemy->setPosition(x, y);
	
	this->addChild(enemy);
	this->m_enemies.pushBack(enemy);

	//移动到屏幕左侧后移除
	auto targetPos = Vec2(-enemy->getContentSize().width/2, enemy->getPositionY());
	auto callFuncN = CallFuncN::create([=](Node *node) {
		auto enemy = dynamic_cast<Sprite *>(node);
		enemy->removeFromParentAndCleanup(true);
		this->m_enemies.eraseObject(enemy);
	});
	ccBezierConfig config;
	config.endPosition = targetPos;
	config.controlPoint_1 = Vec2(visibleSize.width/2, rand() % 2 ? 0 : visibleSize.height);
	config.controlPoint_2 = Vec2(visibleSize.width/2, rand() % 2 ? 0 : visibleSize.height);
	enemy->runAction(Sequence::create(BezierTo::create(5, config), callFuncN, nullptr));
}

bool GameScene::createBullet(Touch *touch, Event *) {
	auto player = this->getChildByTag(1);
	//创建子弹并添加
	auto bullet = Bullet::create();
	bullet->setPosition(player->getPosition());
	this->addChild(bullet);
	this->m_bullets.pushBack(bullet);

	//得到触摸点与英雄的向量差
	auto vec = touch->getLocation() - player->getPosition();
	//得到邻边与对边各自对斜边的比值
	Vec2 normalize = vec.getNormalized();

	//触摸点在右侧，子弹飞出屏幕右侧，否则飞出屏幕左侧
	auto x = visibleSize.width - player->getPositionX() + bullet->getContentSize().width/2;
	if (vec.x < 0)
	{
		x = -bullet->getContentSize().width/2 - player->getPositionX();
	}
	//根据x值求出y坐标：y=x的放大倍数 * y比值
	auto y = x / normalize.x * normalize.y;
	auto targetPos = Vec2(x, y);
	//时间 = 移动距离（斜边） / 速度
	auto time = targetPos.length() / 200;
	auto moveTo = MoveBy::create(time, targetPos);
	auto callFuncN = CallFuncN::create([=](Node *node) {
		auto bullet = dynamic_cast<Bullet *>(node);
		bullet->removeFromParentAndCleanup(true);
		this->m_bullets.eraseObject(bullet);
	});
	bullet->runAction(Spawn::create(Sequence::create(moveTo, callFuncN, nullptr), RotateBy::create(3, 720), nullptr));

	return true;
}

void GameScene::update(float) {
	Vector<Sprite *> removableEnemies;
	Vector<Bullet *> removableBullets;
	for (auto bullet : m_bullets)
	{
		//如果子弹超出上下屏幕边界，提前消除
		if (bullet->getPositionY() < -bullet->getContentSize().height/2
			|| bullet->getPositionY() > visibleSize.height + bullet->getContentSize().height / 2)
		{
			bullet->stopAllActions();
			this->removeChild(bullet);
			removableBullets.pushBack(bullet);
		}

		//子弹与敌人的碰撞检测
		for (auto enemy : m_enemies)
		{
			if (bullet->getBoundingBox().intersectsRect(enemy->getBoundingBox()))
			{
				this->removeChild(enemy);
				this->removeChild(bullet);
				removableBullets.pushBack(bullet);
				removableEnemies.pushBack(enemy);
			}
		}
	}

	for (auto enemy : removableEnemies)
	{
		m_enemies.eraseObject(enemy);
	}
	for (auto bullet : removableBullets)
	{
		m_bullets.eraseObject(bullet);
	}
}

bool GameScene::createFollowBullet(Touch *touch, Event *event) {
	if (m_enemies.size() == 0)
	{
		createBullet(touch, event);
	}
	else
	{
		auto player = this->getChildByTag(1);
		//创建子弹并添加
		auto bullet = Bullet::create();
		bullet->setPosition(player->getPosition());
		this->addChild(bullet);
		this->m_bullets.pushBack(bullet);

		//为这颗子弹随机锁定一个敌机
		auto enemy = m_enemies.at(rand() % m_enemies.size());
		bullet->setClockedEnemy(enemy);
		//分割子弹的运行轨迹，每0.2秒修正一次
		schedule(schedule_selector(GameScene::FollowEnemy), 0.2f);
	}
	return true;
}

//修正子弹轨迹
void GameScene::FollowEnemy(float) {
	//遍历所有的子弹
	for (auto bullet : m_bullets)
	{
		//如果没有锁定敌机，不再本次处理任务之内
		if (!bullet->getClockedEnemy()) continue;
		auto enemy = bullet->getClockedEnemy();
		//如果锁定的敌人已经不在集合中，说明敌人已经不存在了，就重新锁定一个敌人
		if (m_enemies.find(enemy) == m_enemies.end())
		{
			enemy = m_enemies.at(rand() % m_enemies.size());
			bullet->setClockedEnemy(enemy);
		}
		//重新获取子弹与敌机的向量差
		auto vec = enemy->getPosition() - bullet->getPosition();
		//获取邻边和对边各自与斜边的比值
		auto normal = vec.getNormalized();
		//假设斜边就是向量差的距离
		auto len = vec.length();
		//这个距离如果大于40，则只取距离子弹最近的40
		if (len > 40)
		{
			len = 40;
		}
		auto move = MoveBy::create(0.2f, normal * len);
		bullet->runAction(move);
	}
}