all:
	@$(MAKE) -C controller
	@$(MAKE) -C feature_collection
	@$(MAKE) -C utilities
	@$(MAKE) -C ML-Framework

clean:
	@$(MAKE) -C controller clean
	@$(MAKE) -C feature_collection clean
	@$(MAKE) -C utilities clean
	@$(MAKE) -C ML-Framework clean
