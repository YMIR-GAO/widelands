dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   msgctxt = "barbarians_building",
   name = "barbarians_reed_yard",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("barbarians_building", "Reed Yard"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "medium",

   buildcost = {
      log = 5,
      granite = 2
   },
   return_on_dismantle = {
      log = 2,
      granite = 1
   },

   animations = {
      idle = {
         pictures = path.list_files(dirname .. "idle_??.png"),
         hotspot = { 46, 44 },
      },
   },

   aihints = {
      space_consumer = true,
      basic_amount = 1
   },

   working_positions = {
      barbarians_gardener = 1
   },

   outputs = {
      "reed"
   },

   indicate_workarea_overlaps = {
      barbarians_rangers_hut = false,
      barbarians_reed_yard = false,
      barbarians_farm = false,
   },

   programs = {
      work = {
         -- TRANSLATORS: Completed/Skipped/Did not start working because ...
         descname = _"working",
         actions = {
            "call=plant",
            "call=harvest",
            "return=no_stats"
         }
      },
      plant = {
         -- TRANSLATORS: Completed/Skipped/Did not start planting reed because ...
         descname = _"planting reed",
         actions = {
            "callworker=plant",
            "sleep=8000" -- orig sleep=20000 but gardener animation was increased by 2sec
         }
      },
      harvest = {
         -- TRANSLATORS: Completed/Skipped/Did not start harvesting reed because ...
         descname = _"harvesting reed",
         actions = {
            "callworker=harvest",
            "sleep=3000"
         }
      },
   },
   out_of_resource_notification = {
      -- Translators: Short for "Out of ..." for a resource
      title = _"No Fields",
      heading = _"Out of Fields",
      message = pgettext("barbarians_building", "The gardener working at this reed yard has no cleared soil to plant his seeds."),
      productivity_threshold = 20
   },
}
