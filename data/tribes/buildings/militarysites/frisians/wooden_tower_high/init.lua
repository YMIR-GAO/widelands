dirname = path.dirname (__file__)

tribes:new_militarysite_type {
   msgctxt = "frisians_building",
   name = "frisians_wooden_tower_high",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext ("frisians_building", "High Wooden Tower"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "small",
   vision_range = 17,

   enhancement_cost = {
      log = 2,
      thatch_reed = 1
   },
   return_on_dismantle_on_enhanced = {
      log = 1,
   },

   animations = {
      idle = {
         pictures = path.list_files (dirname .. "idle_??.png"),
         hotspot = {40, 94},
         fps = 10,
      },
      unoccupied = {
         pictures = path.list_files (dirname .. "unoccupied_?.png"),
         hotspot = {40, 94},
      },
   },

   aihints = {
      expansion = true,
   },

   max_soldiers = 2,
   heal_per_second = 70,
   conquers = 6,
   prefer_heroes = false,

   messages = {
      occupied = _"Your soldiers have occupied your high wooden tower.",
      aggressor = _"Your high wooden tower discovered an aggressor.",
      attack = _"Your high wooden tower is under attack.",
      defeated_enemy = _"The enemy defeated your soldiers at the high wooden tower.",
      defeated_you = _"Your soldiers defeated the enemy at the high wooden tower."
   },
}
