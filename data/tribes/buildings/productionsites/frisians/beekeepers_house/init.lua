dirname = path.dirname (__file__)

tribes:new_productionsite_type {
   msgctxt = "frisians_building",
   name = "frisians_beekeepers_house",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext ("frisians_building", "Beekeeper’s House"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "small",

   buildcost = {
      brick = 1,
      log = 1,
      reed = 3
   },
   return_on_dismantle = {
      brick = 1,
      reed = 1
   },

   spritesheets = {
      idle = {
         directory = dirname,
         basename = "idle",
         hotspot = {40, 74},
         frames = 10,
         columns = 5,
         rows = 2,
         fps = 10
      }
   },
   animations = {
      unoccupied = {
         directory = dirname,
         basename = "unoccupied",
         hotspot = {40, 56}
      }
   },

   indicate_workarea_overlaps = {
      frisians_berry_farm = true,
      frisians_reed_farm = true,
      frisians_farm = true,
      frisians_beekeepers_house = false,
   },

   aihints = {
      collects_ware_from_map = "honey",
      prohibited_till = 620,
      requires_supporters = true
   },

   working_positions = {
      frisians_beekeeper = 1
   },

   outputs = {
      "honey"
   },

   programs = {
      work = {
         -- TRANSLATORS: Completed/Skipped/Did not start working because ...
         descname = _"working",
         actions = {
            "callworker=bees",
            "sleep=45000"
         }
      },
   },

   out_of_resource_notification = {
      -- Translators: Short for "Out of ..." for a resource
      title = _"No Flowers",
      heading = _"Out of Flowers",
      message = pgettext ("frisians_building", "The beekeeper working at this beekeepers’s house can’t find any flowering fields or bushes in his work area. You should consider building another farm or berry farm nearby, or dismantling or destroying this building."),
      productivity_threshold = 8
   },
}
