# Methane Data Modules

Code of these modules is located in `Methane::Data` namespace:

- [Types](Types) - data storage types like `Chunk`, `Point`, `Rect`
- [RangeSet](RangeSet) - scalar range type `Range` and std::set adaptation `RangeSet`
- [Events](Events) - observer pattern implementation with `Emitter` and `Receiver` base template classes
- [Primitives](Primitives) - primitive data algorithms
- [Provider](Provider) - data provider interface `Provider` and its implementations, including `FileProvider` and `ResourceProvider`
- [Animation](Animation) - animation classes