#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace cocos2d;

struct Challenge {
	int lives = 100;
	int practiceRuns = 3;
	int skips = 3;
	int levels = 0;
	bool active = false;

	bool tempCoin1;
	bool tempCoin2;
	bool tempCoin3;
} challenge;

class $modify(ChallengeBrowser, LevelBrowserLayer) {
	struct Fields {
		static ChallengeBrowser* s_instance;
		static int lastCompletedIndex;
		CCMenuItemSpriteExtra* challengeButton;
		CCLabelBMFont* statusLabel;
	};
	
	bool init(GJSearchObject* p0) {
		LevelBrowserLayer::init(p0);

		this->m_fields->s_instance = this;

		if (challenge.active) {
			this->showChallengeStatus();
			return true;
		}

		if (auto menu = this->getChildByID("refresh-menu")) {
			auto sprite = CCSprite::createWithSpriteFrameName("100LifeButton.png"_spr);
			
			auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(ChallengeBrowser::onChallenge));
			button->setID("hundred-challenge-button");
			button->setPositionY(50.0f);

			this->m_fields->challengeButton = button;
			menu->addChild(button);
		}

		return true;
	}

	void onChallenge(CCObject*) {
		geode::createQuickPopup(
			"100 Life Challenge",
			"Are you sure you want to start the <cr>100 Life Challenge?</c>",
			"No", "Yes",
			[this](auto, bool yesBtn) {
				if (yesBtn) {
					challenge.active = true;
					challenge.lives = 100;
					challenge.practiceRuns = 3;
					challenge.skips = 3;
					challenge.levels = 0;

					if (this->m_fields->challengeButton) {
						this->m_fields->challengeButton->setVisible(false);
					}

					this->showChallengeStatus();
				}
			}
		);
	}

	void showChallengeStatus() {
		auto statusMenu = CCMenu::create();
		statusMenu->setID("100-life-status");

		auto statusLabel = CCLabelBMFont::create(fmt::format("Lives: {}\nPractice Runs: {}\nSkips: {}\nLevels: {}",
		                    challenge.lives, challenge.practiceRuns, challenge.skips, challenge.levels).c_str(), "bigFont.fnt");
		statusLabel->setPosition({ -238.0f, -62.0f });
		statusLabel->setScale(0.3f);

		this->m_fields->statusLabel = statusLabel;

		statusMenu->addChild(statusLabel);
		this->addChild(statusMenu);
	}

	void updateChallengeStatus() {
		if (this->m_fields->statusLabel) {
			std::string updatedStatusText = fmt::format("Lives: {}\nPractice Runs: {}\nSkips: {}\nLevels: {}",
			                                            challenge.lives, challenge.practiceRuns, challenge.skips, challenge.levels);
			this->m_fields->statusLabel->setString(updatedStatusText.c_str());
		}
	}
};

ChallengeBrowser* ChallengeBrowser::Fields::s_instance = nullptr;
int ChallengeBrowser::Fields::lastCompletedIndex = -1;

class $modify(PauseLayer) {
	void onPracticeMode(CCObject* sender) {
		if (!challenge.active) return PauseLayer::onPracticeMode(sender);
		if (challenge.practiceRuns > 0) {
			--challenge.practiceRuns;
			PauseLayer::onPracticeMode(sender);
		} else {
			FLAlertLayer::create("Practice Mode", "You do not have enough practice runs!", "OK")->show();
		}
	}
};

class $modify(LevelCell) {	
	void onClick(CCObject* sender) {
		if (!ChallengeBrowser::Fields::s_instance) return LevelCell::onClick(sender);
		if (!challenge.active) return LevelCell::onClick(sender);
		
		auto level = static_cast<GJGameLevel*>(this->m_level);
		int res = shouldSkipLevel(level);
		if (res == 1) {
			promptSkip(level, [this, sender](bool shouldProceed) {
				if (shouldProceed) {
					LevelCell::onClick(sender);
				}
			});
		} else if (res == 2) {
			LevelCell::onClick(sender);
		}
	}

	int shouldSkipLevel(GJGameLevel* level) {
		if (challenge.skips <= 0) {
			FLAlertLayer::create("No skips", "You are out of skips!", "OK")->show();
			return 0;
		}

		int currentIndex = getLevelIndex(level);
		if (currentIndex > ChallengeBrowser::Fields::lastCompletedIndex + 1) {
			return 1;
		} else {
			return 2;
		}
	}


	int getLevelIndex(GJGameLevel* level) {
		auto browserInstance = ChallengeBrowser::Fields::s_instance;
		auto listLayer = static_cast<GJListLayer*>(browserInstance->getChildByID("GJListLayer"));
		if (!listLayer) return -1;

		auto listView = listLayer->m_listView;
		if (!listView) return -1;

		auto table = listView->m_tableView;
		if (!table) return -1;

		auto content = table->m_contentLayer;
		if (!content) return -1;

		for (int i = 0; i < content->getChildrenCount(); ++i) {
			auto cell = static_cast<LevelCell*>(content->getChildren()->objectAtIndex(i));
			if (cell && cell->m_level == level) {
				return i;
			}
		}

		return -1;
	}

	void promptSkip(GJGameLevel* level, std::function<void(bool)> callback) {
		int currentIndex = getLevelIndex(level);
		geode::createQuickPopup(
			"Skip Level",
			"Are you sure you want to skip this level?",
			"No", "Yes",
			[this, currentIndex, callback](auto, bool yesBtn) {
				if (yesBtn) {
					--challenge.skips;
					ChallengeBrowser::Fields::lastCompletedIndex = currentIndex;
					callback(true);
				} else {
					callback(false);
				}
			}
		);
	}
};

class $modify(PlayLayer) {
	struct Fields {
		std::vector<EffectGameObject*> coins;
	};
	
	void destroyPlayer(PlayerObject* player, GameObject* obj) {
		if (obj == m_anticheatSpike) return PlayLayer::destroyPlayer(player, obj);
		if (this->m_isPracticeMode) return PlayLayer::destroyPlayer(player, obj);
		if (challenge.active && --challenge.lives <= 0) {
			this->onQuit();
			FLAlertLayer::create("Game Over!", "You are out of lives!", "OK")->show();
			challenge.active = false;
			return;
		}
		PlayLayer::destroyPlayer(player, obj);
	}

	void levelComplete() {
		if (challenge.active) {
			ChallengeBrowser::Fields::lastCompletedIndex++;
			challenge.levels++;
		}

		/*
		int coinsAmount = 0;
		geode::log::info("{}", m_fields->coins.size());
		for (int i = 0; i < m_fields->coins.size(); i++) {
			geode::log::info("{}", m_fields->coins[i]);
			if (m_fields->coins[i]->m_opacity == 0) {
				coinsAmount++;
			}
		}*/

		PlayLayer::levelComplete();
	}

	void addObject(GameObject* obj) {
		PlayLayer::addObject(obj);

		if (obj->m_objectType == GameObjectType::UserCoin) {
			m_fields->coins.push_back(static_cast<EffectGameObject*>(obj));
		}
	}
};

class $modify(GJBaseGameLayer) {
	void pickupItem(EffectGameObject* obj) {
		if (obj->m_objectType == GameObjectType::UserCoin) {
		}

		GJBaseGameLayer::pickupItem(obj);
	}
};

class $modify(LevelInfoLayer) {
	void onBack(CCObject* sender) {
		if (challenge.active) {
			ChallengeBrowser::Fields::s_instance->updateChallengeStatus();
		}
		LevelInfoLayer::onBack(sender);
	}
};