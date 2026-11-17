DELETE FROM creature_template WHERE entry >= 760000 AND entry <= 760009;
INSERT INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `speed_swim`, `speed_flight`, `detection_range`, `rank`, `dmgschool`, `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`, `family`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `CreatureImmunitiesId`, `flags_extra`, `ScriptName`, `VerifiedBuild`) VALUES
(760000, 0, 0, 0, 0, 0, 'Druid PVP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 2, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340),
(760001, 0, 0, 0, 0, 0, 'Hunter PVP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 2, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340),
(760002, 0, 0, 0, 0, 0, 'Mage PVP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 8, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340),
(760003, 0, 0, 0, 0, 0, 'Paladin PVP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 2, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340),
(760004, 0, 0, 0, 0, 0, 'Priest PVP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 8, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340),
(760005, 0, 0, 0, 0, 0, 'Rogue PVP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 1, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340),
(760006, 0, 0, 0, 0, 0, 'Shaman PVP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 2, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340),
(760007, 0, 0, 0, 0, 0, 'Warlock PVP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 8, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340),
(760008, 0, 0, 0, 0, 0, 'Warrior PVP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 1, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340),
(760009, 0, 0, 0, 0, 0, 'Death Knight PvP Trainer', '', '', 0, 70, 70, 2, 35, 49, 1.1, 1.14286, 1, 1, 20, 0, 0, 1, 1, 1, 1, 1, 1, 768, 2048, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 'npc_azth_vendor', 12340);

DELETE FROM `creature_template_model` WHERE `CreatureID` IN (760000, 760001, 760002, 760003, 760004, 760005, 760006, 760007, 760008, 760009);
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(760000, 0, 24029, 1, 1, 12340),
(760001, 0, 24030, 1, 1, 12340),
(760002, 0, 24031, 1, 1, 12340),
(760003, 0, 24032, 1, 1, 12340),
(760004, 0, 24033, 1, 1, 12340),
(760005, 0, 23777, 1, 1, 12340),
(760006, 0, 24034, 1, 1, 12340),
(760007, 0, 24035, 1, 1, 12340),
(760008, 0, 24036, 1, 1, 12340),
(760009, 0, 24036, 1, 1, 12340);

UPDATE creature_template SET minlevel = 82, maxlevel = 82 WHERE entry >= 760000 AND entry <= 760009;
