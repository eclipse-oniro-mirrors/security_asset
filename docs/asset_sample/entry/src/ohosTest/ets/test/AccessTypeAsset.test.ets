import asset from '@ohos.security.asset';
import { addAssetPromiseResult, removeAssetPromise, stringToArray, queryAssetPromiseResult, removeAll } from './common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import { BusinessError } from '@ohos.base';
let aliasA1 = 'AAlias1'
let aliasA2 = 'AAlias2'
let aliasC1 = 'CAlias1'
let aliasC2 = 'CAlias2'
let aliasD1 = 'DAlias1'
let aliasD2 = 'DAlias2'
export default function accessTypePromiseTest() {
  describe('accessTypePromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('addAAlias1', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(aliasA1))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_UNLOCKED)
      attr.set(asset.Tag.IS_PERSISTENT, true)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
    })

    it('queryAAlias1', 0, async () => {
      let attr2: asset.AssetMap = new Map();
      attr2.set(asset.Tag.ALIAS, stringToArray(aliasA1))
      attr2.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      try {
        await asset.query(attr2)
        expect(true).assertTrue()
      } catch (error) {
        expect().assertFail()
      }
    })

    it('updateAAlias1', 0, async () => {
      let attr3: asset.AssetMap = new Map();
      attr3.set(asset.Tag.ALIAS, stringToArray(aliasA1))
      let attr4: asset.AssetMap = new Map();
      attr4.set(asset.Tag.SECRET, stringToArray("new_pwd"))
      try {
        await asset.update(attr3, attr4)
        expect(true).assertTrue()
      } catch (error) {
        expect().assertFail()
      }
    })

    it('removeAAlias1', 0, async () => {
      expect(await removeAssetPromise(aliasA1)).assertEqual(true)
    })

    it('addAAlias2Error', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(aliasA2))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_UNLOCKED)
      attr.set(asset.Tag.IS_PERSISTENT, true)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.STATUS_MISMATCH)
    })

    it('queryAAlias1Error', 0, async () => {
      let attr2: asset.AssetMap = new Map();
      attr2.set(asset.Tag.ALIAS, stringToArray(aliasA1))
      attr2.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      expect(await queryAssetPromiseResult(attr2)).assertEqual(asset.ErrorCode.STATUS_MISMATCH)
    })

    it('updateAAlias1Error', 0, async () => {
      let attr3: asset.AssetMap = new Map();
      attr3.set(asset.Tag.ALIAS, stringToArray(aliasA1))
      let attr4: asset.AssetMap = new Map();
      attr4.set(asset.Tag.SECRET, stringToArray("new_pwd"))
      try {
        await asset.update(attr3, attr4)
        expect(false).assertTrue()
      } catch (error) {
        expect(error.code).assertEqual(asset.ErrorCode.STATUS_MISMATCH)
      }
    })

    it('addAAlias2', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(aliasA2))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_UNLOCKED)
      attr.set(asset.Tag.IS_PERSISTENT, true)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
    })

    it('addCAlias1', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(aliasC1))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_FIRST_UNLOCKED)
      attr.set(asset.Tag.IS_PERSISTENT, true)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
    })

    it('queryCAlias1', 0, async () => {
      let attr2: asset.AssetMap = new Map();
      attr2.set(asset.Tag.ALIAS, stringToArray(aliasC1))
      attr2.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      try {
        await asset.query(attr2)
        expect(true).assertTrue()
      } catch (error) {
        expect().assertFail()
      }
    })

    it('updateCAlias1', 0, async () => {
      let attr3: asset.AssetMap = new Map();
      attr3.set(asset.Tag.ALIAS, stringToArray(aliasC1))
      let attr4: asset.AssetMap = new Map();
      attr4.set(asset.Tag.SECRET, stringToArray("new_pwd"))
      try {
        await asset.update(attr3, attr4)
        expect(true).assertTrue()
      } catch (error) {
        expect().assertFail()
      }
    })

    it('removeCAlias1', 0, async () => {
      expect(await removeAssetPromise(aliasC1)).assertEqual(true)
    })

    it('addCAlias2Error', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(aliasC2))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_FIRST_UNLOCKED)
      attr.set(asset.Tag.IS_PERSISTENT, true)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.STATUS_MISMATCH)
    })

    it('queryCAlias1Error', 0, async () => {
      let attr2: asset.AssetMap = new Map();
      attr2.set(asset.Tag.ALIAS, stringToArray(aliasC1))
      attr2.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      expect(await queryAssetPromiseResult(attr2)).assertEqual(asset.ErrorCode.STATUS_MISMATCH)
    })

    it('updateCAlias1Error', 0, async () => {
      let attr3: asset.AssetMap = new Map();
      attr3.set(asset.Tag.ALIAS, stringToArray(aliasC1))
      let attr4: asset.AssetMap = new Map();
      attr4.set(asset.Tag.SECRET, stringToArray("new_pwd"))
      try {
        await asset.update(attr3, attr4)
        expect(false).assertTrue()
      } catch (error) {
        expect(error.code).assertEqual(asset.ErrorCode.STATUS_MISMATCH)
      }
    })

    it('addCAlias2', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(aliasC2))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_FIRST_UNLOCKED)
      attr.set(asset.Tag.IS_PERSISTENT, true)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
    })

    it('addDAlias1', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(aliasD1))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON)
      attr.set(asset.Tag.IS_PERSISTENT, true)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
    })

    it('queryDAlias1', 0, async () => {
      let attr2: asset.AssetMap = new Map();
      attr2.set(asset.Tag.ALIAS, stringToArray(aliasD1))
      attr2.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      try {
        await asset.query(attr2)
        expect(true).assertTrue()
      } catch (error) {
        expect().assertFail()
      }

    })

    it('updateDAlias1', 0, async () => {
      let attr3: asset.AssetMap = new Map();
      attr3.set(asset.Tag.ALIAS, stringToArray(aliasD1))
      let attr4: asset.AssetMap = new Map();
      attr4.set(asset.Tag.SECRET, stringToArray("new_pwd"))
      try {
        await asset.update(attr3, attr4)
        expect(true).assertTrue()
      } catch (error) {
        expect().assertFail()
      }
    })

    it('removeDAlias1', 0, async () => {
      expect(await removeAssetPromise(aliasD1)).assertEqual(true)
    })

    it('addDAlias2', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(aliasD2))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON)
      attr.set(asset.Tag.IS_PERSISTENT, true)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
    })

    it('removeAll', 0, async () => {
      expect(await removeAll()).assertEqual(true)
    })
  })
}