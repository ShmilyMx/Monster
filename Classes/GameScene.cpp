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
	//���Y����Ļ�Ҳ�����ֵ���
	auto minY = enemy->getContentSize().height / 2;
	auto maxY = visibleSize.height - minY;
	auto x = visibleSize.width + enemy->getContentSize().width / 2;
	auto y = rand() % int((maxY - minY)) + minY;
	enemy->setPosition(x, y);
	
	this->addChild(enemy);
	this->m_enemies.pushBack(enemy);

	//�ƶ�����Ļ�����Ƴ�
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
	//�����ӵ������
	auto bullet = Bullet::create();
	bullet->setPosition(player->getPosition());
	this->addChild(bullet);
	this->m_bullets.pushBack(bullet);

	//�õ���������Ӣ�۵�������
	auto vec = touch->getLocation() - player->getPosition();
	//�õ��ڱ���Ա߸��Զ�б�ߵı�ֵ
	Vec2 normalize = vec.getNormalized();

	//���������Ҳ࣬�ӵ��ɳ���Ļ�Ҳ࣬����ɳ���Ļ���
	auto x = visibleSize.width - player->getPositionX() + bullet->getContentSize().width/2;
	if (vec.x < 0)
	{
		x = -bullet->getContentSize().width/2 - player->getPositionX();
	}
	//����xֵ���y���꣺y=x�ķŴ��� * y��ֵ
	auto y = x / normalize.x * normalize.y;
	auto targetPos = Vec2(x, y);
	//ʱ�� = �ƶ����루б�ߣ� / �ٶ�
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
		//����ӵ�����������Ļ�߽磬��ǰ����
		if (bullet->getPositionY() < -bullet->getContentSize().height/2
			|| bullet->getPositionY() > visibleSize.height + bullet->getContentSize().height / 2)
		{
			bullet->stopAllActions();
			this->removeChild(bullet);
			removableBullets.pushBack(bullet);
		}

		//�ӵ�����˵���ײ���
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
		//�����ӵ������
		auto bullet = Bullet::create();
		bullet->setPosition(player->getPosition());
		this->addChild(bullet);
		this->m_bullets.pushBack(bullet);

		//Ϊ����ӵ��������һ���л�
		auto enemy = m_enemies.at(rand() % m_enemies.size());
		bullet->setClockedEnemy(enemy);
		//�ָ��ӵ������й켣��ÿ0.2������һ��
		schedule(schedule_selector(GameScene::FollowEnemy), 0.2f);
	}
	return true;
}

//�����ӵ��켣
void GameScene::FollowEnemy(float) {
	//�������е��ӵ�
	for (auto bullet : m_bullets)
	{
		//���û�������л������ٱ��δ�������֮��
		if (!bullet->getClockedEnemy()) continue;
		auto enemy = bullet->getClockedEnemy();
		//��������ĵ����Ѿ����ڼ����У�˵�������Ѿ��������ˣ�����������һ������
		if (m_enemies.find(enemy) == m_enemies.end())
		{
			enemy = m_enemies.at(rand() % m_enemies.size());
			bullet->setClockedEnemy(enemy);
		}
		//���»�ȡ�ӵ���л���������
		auto vec = enemy->getPosition() - bullet->getPosition();
		//��ȡ�ڱߺͶԱ߸�����б�ߵı�ֵ
		auto normal = vec.getNormalized();
		//����б�߾���������ľ���
		auto len = vec.length();
		//��������������40����ֻȡ�����ӵ������40
		if (len > 40)
		{
			len = 40;
		}
		auto move = MoveBy::create(0.2f, normal * len);
		bullet->runAction(move);
	}
}