#include "main.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace cocos2d;

Challenge challenge;

class $modify(ChallengeBrowser, LevelBrowserLayer) {
	struct Fields {
		static ChallengeBrowser* s_instance;
		static int lastCompletedIndex;
		CCMenuItemSpriteExtra* challengeButton;
		CCMenuItemSpriteExtra* exchangeButton;
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

	void onBack(CCObject* sender) {
		if (challenge.active) {
			geode::createQuickPopup(
				"End Challenge",
				"Are you sure you want to go back? <cr>This will end the current challenge!</c>",
				"No", "Yes",
				[this, sender](auto, bool yesBtn) {
					if (yesBtn) {
						challenge.active = false;
						LevelBrowserLayer::onBack(sender);
					}
				}
			);
		} else {
			LevelBrowserLayer::onBack(sender);
		}
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
					challenge.coins = 0;
					challenge.tempCoins = 0;

					if (this->m_fields->challengeButton) {
						this->m_fields->challengeButton->setVisible(false);
					}

					this->showChallengeStatus();
				}
			}
		);
	}

	void onExchange(CCObject* sender) {
		auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
		geode::createQuickPopup(
			"Coin Exchange",
			"Exchange 3 of your coins for a Practice Run or a Skip!",
			"Skip", "Practice Run",
			[this](auto, bool pRunBtn) {
				if (pRunBtn) {
					challenge.practiceRuns++;
					challenge.coins -= 3;
					updateChallengeStatus();
				} else {
					challenge.skips++;
					challenge.coins -= 3;
					updateChallengeStatus();
				}
			}
		);
	}

	void showChallengeStatus() {
		auto statusMenu = CCMenu::create();
		statusMenu->setID("100-life-status");

		auto statusLabel = CCLabelBMFont::create(fmt::format("Lives: {}\nPractice Runs: {}\nSkips: {}\nLevels: {}\nCoins: {}",
		                    challenge.lives, challenge.practiceRuns, challenge.skips, challenge.levels, challenge.coins).c_str(), "bigFont.fnt");
		statusLabel->setPosition({ -238.0f, 40.0f });
		statusLabel->setScale(0.3f);

		this->m_fields->statusLabel = statusLabel;

		statusMenu->addChild(statusLabel);
		this->addChild(statusMenu);
	}

	void updateChallengeStatus() {
		if (challenge.lives <= 0) {
			m_fields->statusLabel->removeFromParent();
			m_fields->statusLabel = nullptr;

			m_fields->challengeButton->setVisible(true);
		}
		
		if (m_fields->statusLabel) {
			std::string updatedStatusText = fmt::format("Lives: {}\nPractice Runs: {}\nSkips: {}\nLevels: {}\nCoins: {}",
			                                            challenge.lives, challenge.practiceRuns, challenge.skips, challenge.levels, challenge.coins);
			m_fields->statusLabel->setString(updatedStatusText.c_str());

			if (challenge.coins >= 3 && !m_fields->exchangeButton) {
				auto sprite = ButtonSprite::create("Exchange");
				auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(ChallengeBrowser::onExchange));
				button->setID("coin-exchange");
				button->setPosition({ -256.0f, 5.0f });
				button->setScale(0.375f);

				m_fields->exchangeButton = button;

				auto statusMenu = static_cast<CCMenu*>(this->getChildByID("100-life-status"));
				statusMenu->addChild(button);
			} else if (challenge.coins < 3 && m_fields->exchangeButton) {
				m_fields->exchangeButton->removeFromParent();
				m_fields->exchangeButton = nullptr;
			}
		}
	}

	int getLevelIndex(GJGameLevel* level) {
		auto listLayer = static_cast<GJListLayer*>(this->getChildByID("GJListLayer"));
		if (!listLayer) return -1;

		auto listView = listLayer->m_listView;
		if (!listView) return -1;

		auto table = listView->m_tableView;
		if (!table) return -1;

		auto content = table->m_contentLayer;
		if (!content) return -1;

		auto pageEntries = listView->m_entries;
		auto pageNumber = listView->m_currentPage;
		geode::log::info("number: {}", pageNumber);

		for (int i = 0; i < content->getChildrenCount(); ++i) {
			auto cell = static_cast<LevelCell*>(content->getChildren()->objectAtIndex(i));
			if (cell && cell->m_level == level) {
				return i;
			}
		}

		return -1;
	}
};

ChallengeBrowser* ChallengeBrowser::Fields::s_instance = nullptr;
int ChallengeBrowser::Fields::lastCompletedIndex = -1;

class $modify(PauseLayer) {
	void onPracticeMode(CCObject* sender) {
		if (!challenge.active) return PauseLayer::onPracticeMode(sender);
		if (challenge.practiceRuns > 0) {
			--challenge.practiceRuns;
			return PauseLayer::onPracticeMode(sender);
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

		SkipRes res = shouldSkip(level);
		if (res == SkipRes::Skip && challenge.skips > 0) {
			return promptSkip(level, [this, sender](bool shouldProceed) {
				if (shouldProceed) {
					LevelCell::onClick(sender);
					--challenge.skips;
				}
			});
		} else if (res == SkipRes::DontSkip) {
			return LevelCell::onClick(sender);
		} else if (res == SkipRes::SkipAhead) {
			FLAlertLayer::create("Skip ahead", "You are not allowed to skip ahead!", "OK")->show();
		} else if (res == SkipRes::Rebeat) {
			FLAlertLayer::create("Rebeat", "You are not allowed to rebeat a level!", "OK")->show();
		} else if (challenge.skips <= 0) {
			FLAlertLayer::create("Out of skips!", "You are out of skips!", "OK")->show();
		}
	}

	SkipRes shouldSkip(GJGameLevel* level) {
		int currentIndex = ChallengeBrowser::Fields::s_instance->getLevelIndex(level);
		if (currentIndex > ChallengeBrowser::Fields::lastCompletedIndex + 1) {
			if (currentIndex > ChallengeBrowser::Fields::lastCompletedIndex + 2) {
				return SkipRes::SkipAhead;
			}
			return SkipRes::Skip;
		} 
		
		if (currentIndex < ChallengeBrowser::Fields::lastCompletedIndex + 1) {
			return SkipRes::Rebeat;
		}

		return SkipRes::DontSkip;
	}

	void promptSkip(GJGameLevel* level, std::function<void(bool)> callback) {
		int currentIndex = ChallengeBrowser::Fields::s_instance->getLevelIndex(level);
		geode::createQuickPopup(
			"Skip Level",
			"Are you sure you want to skip to this level?",
			"No", "Yes",
			[this, currentIndex, callback](auto, bool yesBtn) {
				if (yesBtn) {
					ChallengeBrowser::Fields::lastCompletedIndex = currentIndex - 1;					
					return callback(true);
				} else {
					return callback(false);
				}
			}
		);
	}
};

class $modify(PlayLayer) {
	void destroyPlayer(PlayerObject* player, GameObject* obj) {
		if (obj == m_anticheatSpike) return PlayLayer::destroyPlayer(player, obj);
		if (this->m_isPracticeMode) return PlayLayer::destroyPlayer(player, obj);
		if (!challenge.active) return PlayLayer::destroyPlayer(player, obj);
		
		challenge.tempCoins = 0;

		if (--challenge.lives <= 0) {
			PlayLayer::pauseGame(true);
			FLAlertLayer::create("Out of lives!", fmt::format("You are out of lives. You survived {} levels!", challenge.levels), "OK")->show();
		}

		PlayLayer::destroyPlayer(player, obj);
	}

	void levelComplete() {
		if (challenge.active && !this->m_isPracticeMode) {
			ChallengeBrowser::Fields::lastCompletedIndex++;
			challenge.levels++;
			challenge.lives += challenge.tempCoins;
			challenge.coins += challenge.tempCoins;
		}

		PlayLayer::levelComplete();
	}
};

class $modify(GJBaseGameLayer) {
	void pickupItem(EffectGameObject* obj) {
		if (obj->m_objectType == GameObjectType::UserCoin && challenge.active) {
			challenge.tempCoins++;
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

	void levelUpdateFailed(int p0) {
		if (!challenge.active) return LevelInfoLayer::levelUpdateFailed(p0);
		
		LevelInfoLayer::keyBackClicked();
		
        int currentIndex = ChallengeBrowser::Fields::s_instance->getLevelIndex(this->m_level);
		ChallengeBrowser::Fields::lastCompletedIndex = currentIndex;

		FLAlertLayer::create("Level Failed", "This level no longer exists.", "OK")->show();
	}
};