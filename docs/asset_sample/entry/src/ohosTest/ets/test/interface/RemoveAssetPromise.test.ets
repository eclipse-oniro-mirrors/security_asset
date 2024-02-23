import asset from '@ohos.security.asset';
import { stringToArray, removeAssetPromise, addAssetPromise, addAssetPromiseResult,
  removeAssetPromiseResult, queryAssetPromiseResult, queryAssetByAlias
} from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

export default function removeAssetPromiseTest() {
  describe('removeFuncPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('removeSingleData', 0, async () => {
      let alias = 'removeSingleData'
      expect(await addAssetPromise(alias)).assertTrue()
      expect(await removeAssetPromise(alias)).assertTrue()
      expect(await queryAssetByAlias(alias)).assertEqual(asset.ErrorCode.NOT_FOUND)
    })

    it('removeMultipleData', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray("removeMultipleData1"))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("normal_label"))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.set(asset.Tag.ALIAS, stringToArray("removeMultipleData2"))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.clear()
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("normal_label"))
      expect(await removeAssetPromiseResult(attr)).assertEqual(0)
      expect(await queryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.NOT_FOUND)
    })

    it('removeAllData', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray("removeAllData1"))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("normal_label"))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.set(asset.Tag.ALIAS, stringToArray("removeAllData2"))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.clear()
      expect(await removeAssetPromiseResult(attr)).assertEqual(0)
      expect(await queryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.NOT_FOUND)
    })

    it('removeDataNotExist', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray('removeDataNotExist'))
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.NOT_FOUND)
      expect(await queryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.NOT_FOUND)
    })
  })

  describe('removeInvalidArgumentPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('removeAliasInvalidLen', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, new Uint8Array(257));
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.ALIAS, new Uint8Array(0));
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('removeAliasUnmatchedType', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, 0);
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.ALIAS, false);
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('removeAccessibilityInvalid', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON - 1)
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_UNLOCKED + 1)
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })


    it('removeAuthTypeInvalid', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.NONE + 1)
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY + 1)
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('removeSyncTypeInvalid', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.SYNC_TYPE, asset.SyncType.NEVER - 1)
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.SYNC_TYPE, (asset.SyncType.THIS_DEVICE | asset.SyncType.TRUSTED_DEVICE) + 1)
      expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('removeLabelInvalidLen', 0, async () => {
      let labels = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4
      ]
      for (let i = 0; i < labels.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(labels[i], new Uint8Array(0))
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        attr.set(labels[i], new Uint8Array(513))
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('removeBoolTagUnmatchedType', 0, async () => {
      let tags = [asset.Tag.REQUIRE_PASSWORD_SET, asset.Tag.IS_PERSISTENT]
      for (let i = 0; i < tags.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(tags[i], 0)
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        attr.set(tags[i], new Uint8Array(1))
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('removeBytesTagUnmatchedType', 0, async () => {
      let labels = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4, asset.Tag.ALIAS
      ]
      for (let i = 0; i < labels.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(labels[i], 0)
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        attr.set(labels[i], true)
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('removeNumberTagWithUnmatchedType', 0, async () => {
      let labels = [asset.Tag.ACCESSIBILITY, asset.Tag.AUTH_TYPE, asset.Tag.SYNC_TYPE]
      for (let i = 0; i < labels.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(labels[i], new Uint8Array(1))
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        attr.set(labels[i], true)
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('removeUnsupportedTag', 0, async () => {
      let tagBytes = [asset.Tag.AUTH_CHALLENGE, asset.Tag.AUTH_TOKEN, asset.Tag.SECRET]
      for (let i = 0; i < tagBytes.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(tagBytes[i], new Uint8Array(1))
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }

      let tagNumber = [asset.Tag.AUTH_VALIDITY_PERIOD, asset.Tag.CONFLICT_RESOLUTION, asset.Tag.RETURN_LIMIT,
        asset.Tag.RETURN_OFFSET, asset.Tag.RETURN_ORDERED_BY, asset.Tag.RETURN_TYPE]
      for (let i = 0; i < tagNumber.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(tagNumber[i], 1)
        expect(await removeAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })
  })
}