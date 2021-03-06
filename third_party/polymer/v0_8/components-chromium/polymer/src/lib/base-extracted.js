

  Polymer.Base = {

    // pluggable features
    // `this` context is a prototype, not an instance
    _addFeature: function(feature) {
      this.extend(this, feature);
    },

    // `this` context is a prototype, not an instance
    registerCallback: function() {
      this._registerFeatures();  // abstract
      this._doBehavior('registered'); // abstract
    },

    createdCallback: function() {
      Polymer.telemetry.instanceCount++;
      this.root = this;
      this._doBehavior('created'); // abstract
      this._initFeatures(); // abstract
    },

    // reserved for canonical behavior
    attachedCallback: function() {
      this.isAttached = true;
      this._doBehavior('attached'); // abstract
    },

    // reserved for canonical behavior
    detachedCallback: function() {
      this.isAttached = false;
      this._doBehavior('detached'); // abstract
    },

    // reserved for canonical behavior
    attributeChangedCallback: function(name) {
      this.setAttributeToProperty(this, name);
      this._doBehavior('attributeChanged', arguments); // abstract
    },

    // copy own properties from `api` to `prototype`
    extend: function(prototype, api) {
      if (prototype && api) {
        Object.getOwnPropertyNames(api).forEach(function(n) {
          this.copyOwnProperty(n, api, prototype);
        }, this);
      }
      return prototype || api;
    },

    copyOwnProperty: function(name, source, target) {
      var pd = Object.getOwnPropertyDescriptor(source, name);
      if (pd) {
        Object.defineProperty(target, name, pd);
      }
    }

  };

  if (Object.__proto__) {
    Polymer.Base.chainObject = function(object, inherited) {
      if (object && inherited && object !== inherited) {
        object.__proto__ = inherited;
      }
      return object;
    };
  } else {
    Polymer.Base.chainObject = function(object, inherited) {
      if (object && inherited && object !== inherited) {
        var chained = Object.create(inherited);
        object = Polymer.Base.extend(chained, object);
      }
      return object;
    };
  }

  Polymer.Base = Polymer.Base.chainObject(Polymer.Base, HTMLElement.prototype);

  // TODO(sjmiles): ad hoc telemetry
  Polymer.telemetry.instanceCount = 0;

