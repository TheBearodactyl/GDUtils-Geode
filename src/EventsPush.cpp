#include "EventsPush.h"
#include "CustomSettings.hpp"
#include "Geode/utils/general.hpp"
#include <queue>
#include <Geode/utils/web.hpp>

std::queue<sio::message::ptr> eventQueue;
bool processingEvents = false;
int zOrder = 200;
EventsPush* EventsPush::create(sio::message::ptr const& data) {
    auto ret = new EventsPush();
    if (ret && ret->init(data)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

const char* getDemonDifficultyIcon(int stars) {
    switch(stars){
        case 3: 
            return "difficulty_07_btn_001.png";
        case 4: 
            return "difficulty_08_btn_001.png";
        case 5: 
            return "difficulty_09_btn_001.png";
        case 6: 
            return "difficulty_10_btn_001.png";
        default:
            return "difficulty_06_btn_001.png";
    }
}
 
const char* getDifficultyIcon(int stars) {
    switch(stars){
        case 10:
            return "difficulty_01_btn_001.png";
        case 20:
            return "difficulty_02_btn_001.png";
        case 30:
            return "difficulty_03_btn_001.png";
        case 40:
            return "difficulty_04_btn_001.png";
        case 50:
            return "difficulty_05_btn_001.png";
        case 60:
            return "difficulty_auto_btn_001.png";
        default:
            return "difficulty_00_btn_001.png";
    }
}

void EventsPush::destroySelf() {
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    auto layer = reinterpret_cast<CCLayer*>(scene->getChildByTag(1932));
    if (layer != nullptr) {
        scene->removeChildByTag(1932);
    }
    EventsPush::eventCompletedCallback(scene);
}
// stole from GDR mod lol 
GJDifficulty getDifficulty(int stars) {
    switch(stars){
        case 10:
            return GJDifficulty::Easy;
        case 20:
            return GJDifficulty::Normal;
        case 30:
            return GJDifficulty::Hard;
        case 40:
            return GJDifficulty::Harder;
        case 50:
            return GJDifficulty::Insane;
        case 60:
            return GJDifficulty::Auto;
        default:
            return GJDifficulty::Auto;
    }
}

GJDifficulty getDemonDifficulty(int stars) {
    switch(stars){
        case 3: 
            return GJDifficulty::DemonEasy;
        case 4: 
            return GJDifficulty::DemonMedium;
        case 5: 
            return GJDifficulty::DemonInsane;
        case 6: 
            return GJDifficulty::DemonExtreme;
        default:
            return GJDifficulty::Demon;
    }
}

std::vector<std::string> split_str(std::string &string, char separator) {
    std::stringstream segmentstream(string);
    std::string segmented;
    std::vector<std::string> splitlist;

    while (std::getline(segmentstream, segmented, separator)) {
        splitlist.push_back(segmented);
    }

    return splitlist;
};

GJGameLevel* EventsPush::convertLevelToJSON(std::string& data) {
    // Robtop to JSON
    GJGameLevel* level = GJGameLevel::create();
#ifndef GEODE_IS_ANDROID64 // for some reason a field isnt known
    auto gamelevelmanager = GameLevelManager::sharedState();
    auto gamestatslevel = GameStatsManager::sharedState();

    std::string auto_level = "0";
    std::string demon = "0";
    auto datas_list = split_str(data, '#');
    std::string level_data = datas_list[0];
    std::string creator_data = datas_list[1];

    // Level data
    auto level_data_list = split_str(level_data, ':');
    if (level_data_list[21] != "") {
        demon = level_data_list[21]; //
    } else {
        demon = "0";
    }
    if (level_data_list[25] != "") {
        auto_level = level_data_list[25]; //
    } else {
        auto_level = "0";
    }

    std::string levelIDStr = level_data_list[1];
    level->m_levelID = std::stoi(levelIDStr);

    level->m_levelName = level_data_list[3];
    level->m_levelDesc = level_data_list[35];
    std::string levelVersionStr = level_data_list[5];
    level->m_levelVersion = std::stoi(levelVersionStr);
    std::string levelUserIDStr = level_data_list[7];
    level->m_userID = std::stoi(levelUserIDStr);

    std::string levelDifficultyDenominatorStr = level_data_list[9];
    level->m_ratings = std::stoi(levelDifficultyDenominatorStr);
    std::string levelDifficultyNumeratorStr = level_data_list[11];
    level->m_ratingsSum = std::stoi(levelDifficultyNumeratorStr);

    if (level_data_list[9] == "10") {
        if (demon == "1") {
            std::string levelDemonDifficultyStr = level_data_list[23];
            level->m_difficulty = getDemonDifficulty(std::stoi(levelDemonDifficultyStr));
        } else {
            if (auto_level == "1") {
                level->m_difficulty = getDifficulty(0);
            } else {
                std::string numeratorDifstr = level_data_list[11];
                level->m_difficulty = getDifficulty(std::stoi(numeratorDifstr));
            }
        }
    }
    std::string levelDownloadsStr = level_data_list[13];
    level->m_downloads = std::stoi(levelDownloadsStr);
    if (level_data_list[53] == "0") {
        std::string levelAudioTrackStr = level_data_list[15];
        level->m_audioTrack = std::stoi(levelAudioTrackStr);
    } else {
        std::string levelSongIDStr = level_data_list[53];
        level->m_songID = std::stoi(levelSongIDStr);
    }

    std::string levelGameVersionStr = level_data_list[17];
    level->m_gameVersion = std::stoi(levelGameVersionStr);
    std::string levelLikesStr = level_data_list[19];
    if (std::stoi(level_data_list[19]) >= 0) {
        level->m_likes = std::stoi(levelLikesStr);
        level->m_dislikes = 0;
    } else {
        level->m_likes = 0;
        level->m_dislikes = 0 - std::stoi(levelLikesStr);
    }
    std::string levelLengthStr = level_data_list[37];
    level->m_levelLength = std::stoi(levelLengthStr);
    
    std::string levelDemonStr = demon;
    level->m_demon = std::stoi(levelDemonStr);
    std::string levelStarsStr = level_data_list[27];
    level->m_stars = std::stoi(levelStarsStr);
    
    if (auto_level == "1") {
        level->m_autoLevel = true;
    } else {
        level->m_autoLevel = false;
    }
    
    std::string levelOriginalStr = level_data_list[39];
    level->m_originalLevel = std::stoi(levelOriginalStr);
    if (level_data_list[41] == "1") {
        level->m_twoPlayerMode = true;
    } else {
        level->m_twoPlayerMode = false;
    }

    std::string levelCoinsStr = level_data_list[43];
    level->m_coins = std::stoi(levelCoinsStr);
    std::string levelCoinsVerifiedStr = level_data_list[45];
    level->m_coinsVerified = std::stoi(levelCoinsVerifiedStr);
    std::string levelStarsRequestedStr = level_data_list[47];
    level->m_starsRequested = std::stoi(levelStarsRequestedStr);
    std::string levelFeaturedStr = level_data_list[29];
    level->m_featured = std::stoi(levelFeaturedStr);
    if (level_data_list[31] == "1") {
        level->m_isEpic = true;
    } else {
        level->m_isEpic = false;
    }
    std::string levelDemonDifficultyStr = level_data_list[23];
    level->m_demonDifficulty = std::stoi(levelDemonDifficultyStr);

    std::string levelObjectCountStr = level_data_list[33];
    level->m_objectCount = std::stoi(levelObjectCountStr);

    level->m_levelNotDownloaded = true;

    std::string levelWorkingTimeStr = level_data_list[49];
    level->m_workingTime = std::stoi(levelWorkingTimeStr);

    CCArray* levelids = gamelevelmanager->m_onlineLevels->allKeys();
    CCObject* obj;
    CCARRAY_FOREACH(levelids, obj) {
        std::string levelid = (static_cast<CCString*>(obj))->getCString();
        if (std::stoi(levelid) == level->m_levelID) {
            auto levelFromSaved = static_cast<GJGameLevel*>(gamelevelmanager->m_onlineLevels->objectForKey(levelid));
            level->m_normalPercent = levelFromSaved->m_normalPercent;
            level->m_newNormalPercent2 = levelFromSaved->m_newNormalPercent2;
            level->m_practicePercent = levelFromSaved->m_practicePercent;
            level->m_levelFavorited = levelFromSaved->m_levelFavorited;
            level->m_levelFolder = levelFromSaved->m_levelFolder;
            level->m_attempts = levelFromSaved->m_attempts;
            level->m_jumps = levelFromSaved->m_jumps;
            level->m_clicks = levelFromSaved->m_clicks;
            level->m_attemptTime = levelFromSaved->m_attemptTime;

            level->m_levelNotDownloaded = levelFromSaved->m_levelNotDownloaded;
            level->m_chk = levelFromSaved->m_chk;
            level->m_isChkValid = levelFromSaved->m_isChkValid;
            level->m_isCompletionLegitimate = levelFromSaved->m_isCompletionLegitimate;
            level->m_isVerified = levelFromSaved->m_isVerified;
            level->m_isUploaded = levelFromSaved->m_isUploaded;
        }
    }

    // Creator data
    auto creator_data_list = split_str(creator_data, ':');
    level->m_creatorName = creator_data_list[1];
    std::string levelAccountIDStr = creator_data_list[2];
    level->m_accountID = std::stoi(levelAccountIDStr);
#endif
    return level;
}

void EventsPush::onClickBtn(CCObject* ret) {
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    auto layer = scene->getChildren()->objectAtIndex(0);
    if (layer == nullptr) return;
    auto events_layer = reinterpret_cast<EventsPush*>(scene->getChildByTag(1932));
    if (events_layer == nullptr) return;
    if (events_layer->level->m_levelID == 0) return;
    std::string layerName = typeid(*layer).name() + 6;
    if (layerName != "PlayLayer" && layerName != "PauseLayer" && layerName != "LevelEditorLayer") { // redirect to level
        if (eventType != EventType::Rate) { // EventType::NA should never happen
            //DailyLevelPage::create(eventType == EventType::Weekly)->show();
            return;
        }
        std::string const& url = "http://www.boomlings.com/database/getGJLevels21.php";
        #ifndef GEODE_IS_MACOS
        int level_id = events_layer->level->m_levelID.value();
        #else // mac os 
        int level_id = events_layer->levelId;
        #endif
        //std::string const& url = "https://clarifygdps.com/getGJLevels21.php";
        std::string const& fields = "secret=Wmfd2893gb7&gameVersion=22&type=0&binaryVersion=35&gdw=0&diff=-&len=-&count=1&str=" + std::to_string(level_id);
        web::AsyncWebRequest()
            .bodyRaw(fields)
            .postRequest()
            .fetch(url).text()
            .then([&](std::string & response) {
                if (response != "-1") {
                    auto scene = CCScene::create();
                    auto layer = LevelInfoLayer::create(EventsPush::convertLevelToJSON(response), false);
                    layer->downloadLevel();
                    scene->addChild(layer);
                    CCDirector::sharedDirector()->pushScene(cocos2d::CCTransitionFade::create(0.5f, scene));
                } else {
                    log::info("Level not found. (-1)");
                }
        }).expect([](std::string const& error) {
            log::error("Error occured while doing a web request: {}", error);
        });
    } else { // copy to clipboard
        #ifndef GEODE_IS_MACOS
        clipboard::write(std::to_string(events_layer->level->m_levelID));
        #else
        clipboard::write(std::to_string(events_layer->levelId));
        #endif
        log::info("Copied ID to clipboard!");
    }
}
bool EventsPush::init(sio::message::ptr const& data) {
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    // type
    // 0 = new rate
    // 1 = daily
    // 2 = weekly

    // rate
    // 0 = unrated / star rate
    // 1 = featured
    // 2 = epic
    // {'demon': 0, 'type': 0, 'starsum': 20, 'stars': 2, 'rate': 2, 'title': 'New Rated Level !', 'level_name': 'Pourquoi', 'level_creator': 'by Jouca', 'sprite': 'GJ_square01.png'}
    auto dataMap = data->get_map();
    std::string sprite_name = dataMap["sprite"]->get_string();
    std::string label_title = dataMap["title"]->get_string();
    std::string level_name_label = dataMap["level_name"]->get_string();
    std::string level_by_label = dataMap["level_creator"]->get_string();
    int isDemon = dataMap["demon"]->get_int();
    int type = dataMap["type"]->get_int();
    int starsum = dataMap["starsum"]->get_int();
    int stars = dataMap["stars"]->get_int();
    int rateType = dataMap["rate"]->get_int();
    int coins = dataMap["coins"]->get_int();
    int areCoinsVerified = dataMap["verified_coins"]->get_int();
    int isPlatformer = dataMap["platformer"]->get_int();
    int level_id = 0;
    if (dataMap.find("level_id") != dataMap.end()) {
        level_id = dataMap["level_id"]->get_int();
        level->m_levelName = level_name_label;
        level->m_coins = coins;
        level->m_coinsVerified = areCoinsVerified;
        level->m_levelID = level_id;
        #ifdef GEODE_IS_MACOS
        this->levelId = level_id;
        #endif
    }
    
    
    auto menu = CCMenu::create();

    bool newRate = Mod::get()->getSettingValue<bool>("newRate");
    bool daily = Mod::get()->getSettingValue<bool>("daily");
    bool weekly = Mod::get()->getSettingValue<bool>("weekly");
    switch (type) {
        case 0: // Rate
            eventType = EventType::Rate;
            break;
        case 1: // Daily
            eventType = EventType::Daily;
            break;
        case 2: // Weekly
            eventType = EventType::Weekly;
            break;
    }
    if (type == 0 && !newRate) {
        EventsPush::eventCompletedCallback(scene);
        return true;
    }
    if (type == 1 && !daily) {
        EventsPush::eventCompletedCallback(scene);
        return true;
    }
    if (type == 2 && !weekly) {
        EventsPush::eventCompletedCallback(scene);
        return true;
    }

    log::debug("EventsPush::init");
    auto director = CCDirector::sharedDirector();
    auto winSize = director->getWinSize();
    //auto label = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    auto bg = cocos2d::extension::CCScale9Sprite::create(sprite_name.c_str(), { .0f, .0f, 80.0f, 80.0f, });
    auto bg_click_spr = cocos2d::extension::CCScale9Sprite::create(sprite_name.c_str(), { .0f, .0f, 80.0f, 80.0f, });
    float lrScale = (float)Mod::get()->getSettingValue<double>("size");
    auto LrSize = CCSize{ 240, 70 }; // * lrScale for both
    bg->setScale(lrScale);
    this->setZOrder(zOrder);
    zOrder = zOrder + 10;
    bg->setContentSize(LrSize);
    bg_click_spr->setContentSize(LrSize);
    // 0 / 0 = bottom left
    // screen width / screen height = top right
    // screen width divided by 2 / screen height divided by 2 = middle
    auto bg_btn = CCMenuItemSpriteExtra::create(bg_click_spr, this, menu_selector(EventsPush::onClickBtn));
    menu->addChild(bg_btn);
    menu->setPosition({ bg->getContentSize().width / 2, bg->getContentSize().height / 2 });
    bg->addChild(menu);
    this->addChild(bg);
    //this->addChild(bg);

    CCSprite* diffFace;
    GJDifficultySprite* mythic = nullptr;
    if (isDemon == 0) {
        diffFace = cocos2d::CCSprite::createWithSpriteFrameName(getDifficultyIcon(starsum));
        mythic = GJDifficultySprite::create(static_cast<int>(getDifficulty(starsum)), static_cast<GJDifficultyName>(0));
    } else {
        diffFace = cocos2d::CCSprite::createWithSpriteFrameName(getDemonDifficultyIcon(starsum));
        mythic = GJDifficultySprite::create(static_cast<int>(getDemonDifficulty(starsum)), static_cast<GJDifficultyName>(0));
    }
    if (!strcmp(level_by_label.c_str(), "UPDATE")) {
        diffFace = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
        diffFace->setPosition({26.f, 37.f});
        diffFace->setScale(.9F);
    } else {
        diffFace->setPosition({26.f, 43.f});
        diffFace->setScale(.8F);
        CCSprite* star = cocos2d::CCSprite::createWithSpriteFrameName("star_small01_001.png");
        CCSprite* moon = cocos2d::CCSprite::createWithSpriteFrameName("moon_small01_001.png");
        CCSprite* featured = cocos2d::CCSprite::createWithSpriteFrameName("GJ_featuredCoin_001.png");
        CCSprite* epic = cocos2d::CCSprite::createWithSpriteFrameName("GJ_epicCoin_001.png");
        CCSprite* legendary = cocos2d::CCSprite::createWithSpriteFrameName("GJ_epicCoin2_001.png");
        mythic->updateFeatureState(static_cast<GJFeatureState>(4));

        std::string starcountstr = std::to_string(stars);
        auto starcount = cocos2d::CCLabelBMFont::create(starcountstr.c_str(), "bigFont.fnt");
        starcount->setPosition({25, 18});
        starcount->setAnchorPoint({1, 0.5});
        starcount->setScale(0.35f);
        bg->addChild(starcount);
        star->setPosition({31, 18});
        star->setScale(.775F);
        moon->setPosition({31, 18});
        moon->setScale(.775F);

        if (isPlatformer == 1) bg->addChild(moon);
        else bg->addChild(star);

        featured->setPosition({26.f, 43.f});
        epic->setPosition({26.f, 43.f});
        legendary->setPosition({26.f, 43.f});
        mythic->setPosition({26.f, 43.f});
        featured->setScale(.8F);
        epic->setScale(.8F);
        legendary->setScale(.8F);
        mythic->setScale(.8F);

        switch (rateType) {
            default: // Rate
                bg->addChild(diffFace);
                break;
            case 1: // Featured
                bg->addChild(featured);
                bg->addChild(diffFace);
                break;
            case 2: // Epic
                bg->addChild(epic);
                bg->addChild(diffFace);
                break;
            case 3: // Legendary
                bg->addChild(legendary);
                bg->addChild(diffFace);

                if (Mod::get()->getSettingValue<bool>("customDifficultyFaces")) {
                    CCSprite* legendaryFace = nullptr;
                    if (isDemon == 0) {
                        if (starsum >= 10) {
                            std::string diffStr = std::to_string(static_cast<int>(getDifficulty(starsum)));
                            auto name = "difficulty_0" + diffStr + "_legendaryIcon.png";
                            legendaryFace = CCSprite::create(Mod::get()->expandSpriteName(name.c_str()));
                        }
                    } else {
                        if (starsum < 6) {
                            std::string diffStr = std::to_string(static_cast<int>(getDemonDifficulty(starsum)));
                            auto name = "difficulty_0" + diffStr + "_legendaryIcon.png";
                            legendaryFace = CCSprite::create(Mod::get()->expandSpriteName(name.c_str()));
                        } else {
                            std::string diffStr = std::to_string(static_cast<int>(getDemonDifficulty(starsum)));
                            auto name = "difficulty_" + diffStr + "_legendaryIcon.png";
                            legendaryFace = CCSprite::create(Mod::get()->expandSpriteName(name.c_str()));
                        }
                    }

                    if (legendaryFace != nullptr) {
                        legendaryFace->setPosition({26.f, 48.f});
                        legendaryFace->setScale(.8F);
                        bg->addChild(legendaryFace);
                    }
                }

                break;
            case 4: // Mythic
                bg->addChild(mythic);

                if (Mod::get()->getSettingValue<bool>("customDifficultyFaces")) {
                    CCSprite* mythicFace = nullptr;
                    if (isDemon == 0) {
                        if (starsum >= 10) {
                            std::string diffStr = std::to_string(static_cast<int>(getDifficulty(starsum)));
                            auto name = "difficulty_0" + diffStr + "_mythicIcon.png";
                            mythicFace = CCSprite::create(Mod::get()->expandSpriteName(name.c_str()));
                        }
                    } else {
                        if (starsum < 6) {
                            std::string diffStr = std::to_string(static_cast<int>(getDemonDifficulty(starsum)));
                            auto name = "difficulty_0" + diffStr + "_mythicIcon.png";
                            mythicFace = CCSprite::create(Mod::get()->expandSpriteName(name.c_str()));
                        } else {
                            std::string diffStr = std::to_string(static_cast<int>(getDemonDifficulty(starsum)));
                            auto name = "difficulty_" + diffStr + "_mythicIcon.png";
                            mythicFace = CCSprite::create(Mod::get()->expandSpriteName(name.c_str()));
                        }
                    }

                    if (mythicFace != nullptr) {
                        mythicFace->setPosition({26.f, 48.f});
                        mythicFace->setScale(.8F);
                        bg->addChild(mythicFace);
                    }
                }

                break;
        }
    }
    
    auto node = CCNode::create();
    
    auto title = cocos2d::CCLabelBMFont::create(label_title.c_str(), "goldFont.fnt");
    title->setPosition({ -27, 23 });
    title->setScale(.575F);
    title->setAnchorPoint({ 0, 0.5 });
    node->addChild(title);

    auto level_title = cocos2d::CCLabelBMFont::create(level_name_label.c_str(), "bigFont.fnt");
    level_title->setPosition({ -27, 3 });
    level_title->setScale(.46F);
    
    level_title->setAnchorPoint({ 0, 0.5 });
    if (strcmp(level_by_label.c_str(), "UPDATE")) {
        auto level_by = cocos2d::CCLabelBMFont::create(level_by_label.c_str(), "goldFont.fnt");
        level_by->setPosition({-27, -11 });
        level_by->setScale(.46F);
        level_by->limitLabelWidth(120, 0.46f, 0.1f);
        level_by->setAnchorPoint({ 0, 0.5 });
        node->addChild(level_by);
        level_title->limitLabelWidth(120, 0.46f, 0.1f);
    } else {
        level_title->setPositionY(-1);
        level_title->setScale(.3F);
        //level_title->setAlignment(kCCTextAlignmentCenter);
    }

    node->addChild(level_title);

    auto verifiedCoinSpr1 = cocos2d::CCSprite::createWithSpriteFrameName("GJ_coinsIcon2_001.png");
    verifiedCoinSpr1->setScale(0.35f);
    auto verifiedCoinSpr2 = cocos2d::CCSprite::createWithSpriteFrameName("GJ_coinsIcon2_001.png");
    verifiedCoinSpr2->setScale(0.35f);
    auto verifiedCoinSpr3 = cocos2d::CCSprite::createWithSpriteFrameName("GJ_coinsIcon2_001.png");
    verifiedCoinSpr3->setScale(0.35f);

    if (areCoinsVerified == 0) {
        verifiedCoinSpr1->setColor({255, 175, 75});
        verifiedCoinSpr2->setColor({255, 175, 75});
        verifiedCoinSpr3->setColor({255, 175, 75});
    }

    if (coins == 1 || coins == 3) {
        verifiedCoinSpr2->setPosition({ -52, -22 });
        node->addChild(verifiedCoinSpr2);

        if (coins == 3) {
            verifiedCoinSpr1->setPosition({ -60, -22 });
            verifiedCoinSpr3->setPosition({ -44, -22 });

            node->addChild(verifiedCoinSpr1);
            node->addChild(verifiedCoinSpr3);
        }
    } else if (coins == 2) {
        verifiedCoinSpr1->setPosition({ -55, -22 });
        verifiedCoinSpr2->setPosition({ -48, -22 });

        node->addChild(verifiedCoinSpr1);
        node->addChild(verifiedCoinSpr2);
    }

    if (type > 0) {
        CCSprite* crown;
        if (type == 1) {
            crown = cocos2d::CCSprite::createWithSpriteFrameName("gj_dailyCrown_001.png");
        } else {
            crown = cocos2d::CCSprite::createWithSpriteFrameName("gj_weeklyCrown_001.png");
        }
        crown->setScale(.45F);
        crown->setPosition({ 45, 45 });
        node->addChild(crown);
    }

    node->setAnchorPoint({ 0.5, 0.5 });
    node->setPosition({ 77, 30 });

    bg->addChild(node);

    this->setTag(1932);

    // Move action
    //CCDelayTime
    
    float delayTime = (float)Mod::get()->getSettingValue<double>("time");

    int cornerId = Mod::get()->getSettingValue<SettingPosStruct>("notificationPlacement").m_pos;//ConfigHandler::readConfigInt("notificationPlacement");
    float moveX = .0F;
    switch (cornerId) {
        case 1: // top left
            bg->setPosition((-(bg->getContentSize().width / 2)) * lrScale, ((winSize.height - (bg->getContentSize().height / 2)) - (20 * lrScale)));
            moveX = (bg->getContentSize().width) * lrScale;
            break;
        case 2: // top right
            bg->setPosition(winSize.width + ((bg->getContentSize().width / 2) * lrScale), ((winSize.height - (bg->getContentSize().height / 2)) - (20 * lrScale)));
            moveX = -(bg->getContentSize().width * lrScale);
            break;
        case 3: // bottom left
            bg->setPosition((-(bg->getContentSize().width / 2)) * lrScale, (bg->getContentSize().height / 2) * lrScale);
            moveX = (bg->getContentSize().width) * lrScale;
            break;
        case 4: // bottom right
            ///bg->setPosition((winSize.width + (bg->getContentSize().width)) * lrScale, (bg->getContentSize().height / 2) * lrScale);
            bg->setPosition({ winSize.width + ((bg->getContentSize().width / 2) * lrScale), (bg->getContentSize().height / 2) * lrScale});
            //bg->setPosition((bg->getContentSize().width) * lrScale, (bg->getContentSize().height / 2) * lrScale);
            //moveX = -(bg->getContentSize().width * lrScale);
            moveX = -((bg->getContentSize().width) * lrScale);
            break;
    }
    
    bg->runAction(CCSequence::create(
        CCEaseOut::create(CCMoveBy::create(0.5f, { moveX, 0.0f }), 0.6f),
        CCDelayTime::create(delayTime),
        CCEaseIn::create(CCMoveBy::create(0.5f, { -moveX, 0.0f }), 0.6f),
        CCDelayTime::create(0.5F),
        CCCallFunc::create(this, callfunc_selector(EventsPush::destroySelf)),
        nullptr
    ));

    if (Mod::get()->getSettingValue<bool>("sfx")) FMODAudioEngine::sharedEngine()->playEffect("crystal01.ogg");

    return true;
}

void EventsPush::stopNow(CCScene* scene) {
    EventsPush::eventCompletedCallback(scene);
}

void EventsPush::pushRateLevel(CCScene* self, sio::message::ptr const& data) {
    log::info("pushing rate level");
    // Enqueue the event
    eventQueue.push(data);
    
    // If no event is being processed, start processing events
    if (!processingEvents) {
        EventsPush::processNextEvent(self);
    }
    //auto layer = EventsPush::create(data);
    //self->addChild(layer);
}

void EventsPush::processNextEvent(CCScene* self) {
    if (!eventQueue.empty()) {
        auto data = eventQueue.front();
        eventQueue.pop();
        processingEvents = true;
        
        auto layer = EventsPush::create(data);
        // Set a callback function that will be called when the event is completed
        //layer->setEventCompletedCallback(std::bind(&EventsPush::eventCompletedCallback, this));
        self->addChild(layer);
    }
}

void EventsPush::eventCompletedCallback(CCScene* self) {
    // Event is completed, set processingEvents to false
    processingEvents = false;
    
    // Process the next event (if any)
    EventsPush::processNextEvent(self);
}
