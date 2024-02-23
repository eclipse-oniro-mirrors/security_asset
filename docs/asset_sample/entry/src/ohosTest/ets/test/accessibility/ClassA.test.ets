import asset from '@ohos.security.asset';
import { stringToArray, addAssetPromiseResult, removeAll, queryContentPromise, updateAssetByAlias,
  removeAssetPromise } from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

export default function classA() {
  describe('classATest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('addClassAData', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray('ClassAData'))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_UNLOCKED)
      attr.set(asset.Tag.IS_PERSISTENT, true)
      attr.set(asset.Tag.CONFLICT_RESOLUTION, asset.ConflictResolution.OVERWRITE)

      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.STATUS_MISMATCH)
      // expect(await addAssetPromiseResult(attr)).assertEqual(0)
    })

    it('queryClassAData', 0, async () => {
      expect(await queryContentPromise('ClassAData')).assertEqual(0)
    })

    it('updateClassAData', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd_new"))
      expect(await updateAssetByAlias('ClassAData', attr)).assertEqual(0)
    })

    it('deleteClassAData', 0, async () => {
      expect(await removeAssetPromise('ClassAData')).assertEqual(0)
    })
  })
}