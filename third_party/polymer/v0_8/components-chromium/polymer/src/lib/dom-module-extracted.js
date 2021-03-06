

(function() {

  var modules = {};

  var DomModule = function() {
    return document.createElement('dom-module');
  };

  DomModule.prototype = Object.create(HTMLElement.prototype);

  DomModule.prototype.constructor = DomModule;

  DomModule.prototype.createdCallback = function() {
    var id = this.id || this.getAttribute('name') || this.getAttribute('is');
    if (id) {
      this.id = id;
      modules[id] = this;
    }
  };

  DomModule.prototype.import = function(id, slctr) {
    var m = modules[id];
    if (!m) {
      // If polyfilling, a script can run before a dom-module element
      // is upgraded. We force the containing document to upgrade
      // and try again to workaround this polyfill limitation.
      forceDocumentUpgrade();
      m = modules[id];
    }
    if (m && slctr) {
      m = m.querySelector(slctr);
    }
    return m;
  };

  // NOTE: HTMLImports polyfill does not
  // block scripts on upgrading elements. However, we want to ensure that
  // any dom-module in the tree is available prior to a subsequent script
  // processing.
  // Therefore, we force any dom-modules in the tree to upgrade when dom-module
  // is registered by temporarily setting CE polyfill to crawl the entire
  // imports tree. (Note: this should only upgrade any imports that have been
  // loaded by this point. In addition the HTMLImports polyfill should be
  // changed to upgrade elements prior to running any scripts.)
  var cePolyfill = window.CustomElements && !CustomElements.useNative;
  if (cePolyfill) {
    var ready = CustomElements.ready;
    CustomElements.ready = true;
  }
  document.registerElement('dom-module', DomModule);
  if (cePolyfill) {
    CustomElements.ready = ready;
  }

  function forceDocumentUpgrade() {
    if (cePolyfill) {
      var script = document._currentScript || document.currentScript;
      if (script) {
        CustomElements.upgradeAll(script.ownerDocument);
      }
    }
  }

})();

