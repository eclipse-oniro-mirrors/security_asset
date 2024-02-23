import asset from '@ohos.security.asset';
import { stringToArray, addAssetPromiseResult, queryAssetPromiseResult, removeAll,
  preQueryAssetPromiseResult,
  preQueryAssetPromise,
  postQueryAssetPromise
} from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

export default function preQueryAssetPromiseTest() {
  describe('preQueryFuncPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(async () => {
      expect(await removeAll()).assertTrue()
    })
    afterAll(async () => {
    })
    it('preQuerySingleData', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('preQuerySingleData'))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.delete(asset.Tag.SECRET)
      let challenge: Uint8Array = await preQueryAssetPromise(attr)
      expect(challenge.length).assertEqual(32)
      expect(await postQueryAssetPromise(challenge)).assertTrue()
    })

    it('preQueryBatchData', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('preQueryBatchData1'))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      attr.set(asset.Tag.ALIAS, stringToArray('preQueryBatchData2'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.clear()
      let challenge: Uint8Array = await preQueryAssetPromise(attr)
      expect(challenge.length).assertEqual(32)
      expect(await postQueryAssetPromise(challenge)).assertTrue()
    })

    it('preQueryMaxCrypto', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('preQueryBatchData1'))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.clear()
      let challenges: Array<Uint8Array> = new Array();
      for (let i = 0; i < 16; i++) {
        let challenge: Uint8Array = await preQueryAssetPromise(attr)
        expect(challenge.length).assertEqual(32)
        challenges.push(challenge)
      }

      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.LIMIT_EXCEEDED)
      for (let i = 0; i < 16; i++) {
        expect(await postQueryAssetPromise(challenges[i])).assertTrue()
      }
    })

    it('preQueryDifferentType', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray('preQueryDifferentType1'));
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'));
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON)
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.set(asset.Tag.ALIAS, stringToArray('preQueryDifferentType2'));
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_FIRST_UNLOCKED)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.UNSUPPORTED)
    })
  })

  describe('preQueryInvalidArgumentPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(async () => {
      expect(await removeAll()).assertTrue()
    })
    afterAll(async () => {
    })
    it('preQueryNotExistData', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('preQueryNotExistData'))
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.NOT_FOUND)
    })

    it('preQueryUnsupportedAuthType', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.NONE)
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('preQueryInvalidAliasLen', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, new Uint8Array(0))
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.ALIAS, new Uint8Array(257))
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('preQueryInvalidAccessibility', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON - 1)
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_UNLOCKED + 1)
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('preQueryInvalidAuthType', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.NONE + 1)
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY + 1)
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('preQueryInvalidSyncType', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.SYNC_TYPE, asset.SyncType.NEVER - 1)
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.AUTH_TYPE, (asset.SyncType.THIS_DEVICE | asset.SyncType.TRUSTED_DEVICE) + 1)
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('preQueryInvalidAuthPeriod', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.AUTH_VALIDITY_PERIOD, 0)
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.AUTH_VALIDITY_PERIOD, 601)
      expect(await preQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('preQueryInvalidLabelLen', 0, async () => {
      let labels = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4
      ]
      for (let i = 0; i < labels.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(labels[i], new Uint8Array(0))
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(labels[i], new Uint8Array(513))
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('preQueryBoolUnmatchedType', 0, async () => {
      let tags = [
        asset.Tag.REQUIRE_PASSWORD_SET, asset.Tag.IS_PERSISTENT
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], new Uint8Array(1))
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], 0)
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('preQueryBytesUnmatchedType', 0, async () => {
      let tags = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4, asset.Tag.ALIAS
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], 0)
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], false)
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('preQueryNumberUnmatchedType', 0, async () => {
      let tags = [
        asset.Tag.ACCESSIBILITY, asset.Tag.AUTH_TYPE, asset.Tag.SYNC_TYPE, asset.Tag.AUTH_VALIDITY_PERIOD
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], new Uint8Array(1))
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], false)
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('preQueryUnsupportedTag', 0, async () => {
      let tags = [ asset.Tag.SECRET, asset.Tag.AUTH_CHALLENGE, asset.Tag.AUTH_TOKEN ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], new Uint8Array(1))
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }

      let numberTags = [asset.Tag.RETURN_LIMIT, asset.Tag.RETURN_TYPE, asset.Tag.RETURN_ORDERED_BY, asset.Tag.RETURN_OFFSET, asset.Tag.CONFLICT_RESOLUTION ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(numberTags[i], 1)
        expect(await preQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })
  })
}