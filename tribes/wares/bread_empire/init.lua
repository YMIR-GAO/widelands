dirname = path.dirname(__file__)

tribes:new_ware_type{
   name = "bread_empire",
   -- TRANSLATORS: This is a ware name used in lists of wares
   descname = _"Bread",
   -- TRANSLATORS: mass description, e.g. 'The economy needs ...'
   genericname = _"bread",
   default_target_quantity = {
		empire = 20
	},
   preciousness = {
		empire = 4
	},
   helptext = {
		-- TRANSLATORS: Helptext for a ware: Bread
		empire = _"The bakers of the Empire make really tasty bread out of flour and water. It is used in taverns and inns to prepare rations and meals. Bread is also consumed at the training sites (arena, colosseum, training camp)."
   },
   animations = {
      idle = {
         pictures = { dirname .. "idle.png" },
         hotspot = { 5, 11 },
      },
   }
}
