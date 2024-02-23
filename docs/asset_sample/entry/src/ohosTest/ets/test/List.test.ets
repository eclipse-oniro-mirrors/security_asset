import addAssetPromiseTest from './interface/addAssetPromise.test';
import removeAssetPromiseTest from './interface/RemoveAssetPromise.test';
import updateAssetPromiseTest from './interface/UpdateAssetPromise.test';
import queryAssetPromiseTest from './interface/QueryAssetPromise.test';
import performanceAssetPromiseTest from './PerformanceAsset.test';
import interfaceConcurrencyPromiseTest from './InterfaceConcurrencyAsset.test'
import interfacePressurePromiseTest from './InterfacePressureAsset.test'
import singleInterfacePressurePromiseTest from './SingleInterfacePressureAsset.test';
import classA from './accessibility/ClassA.test';
import preQueryAssetPromiseTest from './interface/PreQueryAssetPromist.test';
import postQueryAssetPromiseTest from './interface/PostQueryAssetPromise.test';
import accessTypePromiseTest from './AccessTypeAsset.test';
import addAssetSyncTest from './interface/AddAssetSync.test';
import postQueryAssetSyncTest from './interface/PostQueryAssetSync.test';
import removeAssetSyncTest from './interface/RemoveAssetSync.test';
import preQueryAssetSyncTest from './interface/PreQueryAssetSync.test';
import queryAssetSyncTest from './interface/QueryAssetSync.test';
import updateAssetSyncTest from './interface/UpdateAssetSync.test';


export default function testsuite() {
  addAssetPromiseTest();
  addAssetSyncTest();
  removeAssetPromiseTest();
  removeAssetSyncTest();
  preQueryAssetPromiseTest();
  preQueryAssetSyncTest();
  queryAssetPromiseTest();
  queryAssetSyncTest();
  postQueryAssetPromiseTest();
  postQueryAssetSyncTest();
  updateAssetPromiseTest();
  updateAssetSyncTest();
  performanceAssetPromiseTest();
  interfaceConcurrencyPromiseTest();
  interfacePressurePromiseTest();
  singleInterfacePressurePromiseTest();
  accessTypePromiseTest();
  classA();
}